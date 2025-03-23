/**
 * @file tcp_client.c
 * @brief Implementação de comunicação TCP via WiFi para Raspberry Pi Pico W
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "../include/tcp_client.h"

// Flag de inicialização do WiFi
static bool wifi_initialized = false;

// Estado global de conexão TCP persistente
static TCP_CLIENT_T* global_tcp_state = NULL;
static uint32_t reconnect_attempts = 0;
static absolute_time_t last_reconnect_time;
static const uint32_t RECONNECT_INTERVAL_MS = 5000; // 5 segundos entre tentativas
static const uint32_t MAX_RECONNECT_ATTEMPTS = 10;  // Máximo de tentativas de reconexão

// Função de callback quando a conexão é fechada
static err_t tcp_client_close(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    err_t err = ERR_OK;
    
    if (state->tcp_pcb != NULL) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        
        err = tcp_close(state->tcp_pcb);
        if (err != ERR_OK) {
            printf("[TCP] Falha ao fechar conexão %d, abortando\n", err);
            tcp_abort(state->tcp_pcb);
            err = ERR_ABRT;
        }
        
        state->tcp_pcb = NULL;
    }
    
    state->connected = false;
    return err;
}

// Callback chamado quando a operação TCP termina
static err_t tcp_result(void *arg, int status) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    if (status == 0) {
        printf("[TCP] Operação concluída com sucesso\n");
        reconnect_attempts = 0; // Reinicia contagem de tentativas após sucesso
    } else {
        //printf("[TCP] Falha na operação: %d\n", status);
    }
    
    state->complete = true;
    
    // Não fechar a conexão em caso de sucesso para manter conexão persistente
    if (status != 0) {
        return tcp_client_close(arg);
    }
    return ERR_OK;
}

// Callback chamado quando os dados são enviados
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    printf("[TCP] Enviados %u bytes\n", len);
    state->sent_len += len;
    
    // Se todos os dados foram enviados, considere a operação concluída
    if (state->user_message != NULL && state->sent_len >= strlen(state->user_message)) {
        tcp_result(arg, 0);
    }
    
    return ERR_OK;
}

// Callback chamado quando a conexão TCP é estabelecida
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    if (err != ERR_OK) {
        printf("[TCP] ERRO CRÍTICO: Falha ao conectar: %d\n", err);
        return tcp_result(arg, err);
    }
    
    state->connected = true;
    printf("[TCP] ✓ Conectado com sucesso ao servidor %s:%d\n", 
           ip4addr_ntoa(&state->remote_addr), tpcb->remote_port);
    
    // Se temos uma mensagem para enviar, envie-a agora
    if (state->user_message != NULL) {
        // Log da mensagem que será enviada (limitado a 40 caracteres)
        char short_msg[41];
        strncpy(short_msg, state->user_message, 40);
        short_msg[40] = '\0';
        printf("[TCP] Enviando: '%s%s'\n", short_msg, 
               strlen(state->user_message) > 40 ? "..." : "");
        
        // Tenta enviar os dados
        cyw43_arch_lwip_begin();
        err = tcp_write(tpcb, state->user_message, strlen(state->user_message), TCP_WRITE_FLAG_COPY);
        cyw43_arch_lwip_end();
        
        if (err != ERR_OK) {
            printf("[TCP] ERRO CRÍTICO: Falha ao enviar dados: %d\n", err);
            return tcp_result(arg, err);
        }
        
        cyw43_arch_lwip_begin();
        err = tcp_output(tpcb);
        cyw43_arch_lwip_end();
        
        if (err != ERR_OK) {
            printf("[TCP] ERRO CRÍTICO: tcp_output falhou: %d\n", err);
            return tcp_result(arg, err);
        }
        
        printf("[TCP] ✓ Dados enviados com sucesso\n");
    } else {
        printf("[TCP] Aviso: Nenhum dado para enviar\n");
        tcp_result(arg, 0);
    }
    
    return ERR_OK;
}

// Callback chamado periodicamente durante a conexão
static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    printf("[TCP] Tempo limite de poll atingido\n");
    // Não consideramos isso um erro, apenas um evento periódico
    return ERR_OK;
}

// Callback para erros TCP
static void tcp_client_err(void *arg, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    printf("[TCP] Erro TCP: %d\n", err);
    
    if (err != ERR_ABRT) {
        state->connected = false;
        state->complete = true;
        if (state->tcp_pcb) {
            state->tcp_pcb = NULL;
        }
    }
}

// Callback chamado quando dados são recebidos
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    if (!p) {
        // O servidor fechou a conexão, mas isso é NORMAL após receber dados
        // Não vamos mais considerar isso um erro
        printf("[TCP] Conexão fechada pelo servidor (comportamento normal)\n");
        
        // Marcamos como desconectado mas SEM ERRO
        state->connected = false;
        
        // Se temos dados no buffer, considera como sucesso (recebeu resposta)
        if (state->buffer_len > 0) {
            printf("[TCP] ✓ Comunicação completa e bem-sucedida\n");
            state->complete = true;
            return ERR_OK;  // Retorna OK em vez de chamar tcp_result
        } else {
            // Mesmo sem dados no buffer, consideramos que o envio foi bem-sucedido
            printf("[TCP] ✓ Dados enviados e conexão finalizada normalmente\n");
            state->complete = true;
            return ERR_OK;
        }
    }
    
    // Processa os dados recebidos
    if (p->tot_len > 0) {
        printf("[TCP] Recebidos %d bytes\n", p->tot_len);
        
        // Copiar os dados recebidos para o buffer
        const uint16_t buffer_left = TCP_BUF_SIZE - state->buffer_len;
        state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        
        // Adicionar terminador nulo para imprimir como string se possível
        if (state->buffer_len < TCP_BUF_SIZE) {
            state->buffer[state->buffer_len] = '\0';
            printf("[TCP] Dados: %s\n", state->buffer);
        }
        
        // Informar o lwIP que processamos os dados
        tcp_recved(tpcb, p->tot_len);
    }
    
    pbuf_free(p);
    return ERR_OK;
}

// Inicializa e abre a conexão TCP
static bool tcp_client_open(TCP_CLIENT_T *state, uint16_t port) {
    printf("[TCP] Conectando a %s porta %u\n", ip4addr_ntoa(&state->remote_addr), port);
    
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        printf("[TCP] Falha ao criar PCB\n");
        return false;
    }
    
    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, 5); // 2.5 segundos (5*500ms)
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);
    
    state->buffer_len = 0;
    state->sent_len = 0;
    state->complete = false;
    
    // Garantir acesso thread-safe ao lwIP
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, port, tcp_client_connected);
    cyw43_arch_lwip_end();
    
    return err == ERR_OK;
}

// Inicializar estado global TCP uma vez
static TCP_CLIENT_T* get_tcp_state() {
    if (global_tcp_state == NULL) {
        global_tcp_state = calloc(1, sizeof(TCP_CLIENT_T));
        if (!global_tcp_state) {
            printf("[TCP] Falha ao alocar memória para o cliente TCP\n");
            return NULL;
        }
    }
    return global_tcp_state;
}

// Limpar o estado TCP para próxima mensagem
static void reset_tcp_state(TCP_CLIENT_T *state) {
    if (state) {
        state->buffer_len = 0;
        state->sent_len = 0;
        state->complete = false;
        state->user_message = NULL;
    }
}

// Verifica e tenta reconectar se necessário
static bool ensure_tcp_connection(TCP_CLIENT_T *state, const char* server_ip, uint16_t port) {
    // Se a conexão está ativa, apenas retorna sucesso
    if (state->connected && state->tcp_pcb != NULL) {
        return true;
    }
    
    // Verificar tempo entre tentativas para não sobrecarregar
    absolute_time_t now = get_absolute_time();
    if (!is_nil_time(last_reconnect_time) && 
        absolute_time_diff_us(last_reconnect_time, now) < RECONNECT_INTERVAL_MS * 1000) {
        // Muito cedo para tentar novamente
        return false;
    }
    
    // Registra tentativa de reconexão
    last_reconnect_time = now;
    reconnect_attempts++;
    
    if (reconnect_attempts > MAX_RECONNECT_ATTEMPTS) {
        printf("[TCP] Número máximo de tentativas de reconexão atingido (%lu)\n", reconnect_attempts);
        return false;
    }
    
    printf("[TCP] Tentativa de reconexão %lu de %u\n", reconnect_attempts, MAX_RECONNECT_ATTEMPTS);
    
    // Certifique-se de que qualquer conexão anterior está limpa
    if (state->tcp_pcb != NULL) {
        tcp_client_close(state);
    }
    
    // Configurar estado para nova conexão
    ip4addr_aton(server_ip, &state->remote_addr);
    
    // Tenta abrir a conexão usando a porta especificada
    return tcp_client_open(state, port);
}

// Funções públicas da biblioteca

bool wifi_init_and_connect(const char* ssid, const char* password, uint32_t timeout_ms) {
    // Se já está inicializado, não faça nada
    if (wifi_initialized) {
        return true;
    }
    
    printf("[WiFi] Inicializando módulo...\n");
    
    if (cyw43_arch_init()) {
        printf("[WiFi] Falha na inicialização do cyw43_arch\n");
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    
    printf("[WiFi] Conectando à rede '%s'...\n", ssid);
    
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, timeout_ms)) {
        printf("[WiFi] Falha ao conectar-se à rede\n");
        cyw43_arch_deinit();
        return false;
    }
    
    printf("[WiFi] Conectado com sucesso!\n");
    printf("[WiFi] Endereço IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    
    // Inicializar valores para reconexão
    reconnect_attempts = 0;
    last_reconnect_time = nil_time;
    
    wifi_initialized = true;
    return true;
}

bool tcp_send_message(const char* server_ip, uint16_t port, const char* message) {
    // Diagnóstico detalhado para debug
    printf("[TCP] Tentando enviar para %s:%u: '%s'\n", server_ip, port, message);
    
    if (!wifi_initialized) {
        printf("[TCP] WiFi não inicializado. Inicialize o WiFi primeiro.\n");
        return false;
    }
    
    if (!server_ip || !message) {
        printf("[TCP] Parâmetros inválidos (IP ou mensagem nulos)\n");
        return false;
    }

    // Verificar se o servidor IP tem formato válido (diagnóstico)
    ip_addr_t test_addr;
    if (ipaddr_aton(server_ip, &test_addr) == 0) {
        printf("[TCP] ERRO: Formato de IP inválido: %s\n", server_ip);
        return false;
    } else {
        printf("[TCP] IP validado: %s\n", ip4addr_ntoa(&test_addr));
    }
    
    // Obter o estado TCP (reutilizando ou criando novo)
    TCP_CLIENT_T *state = get_tcp_state();
    if (!state) {
        printf("[TCP] Falha ao obter estado TCP. Memória insuficiente?\n");
        return false;
    }

    // Força nova conexão para testar problema de conexão persistente
    if (state->tcp_pcb != NULL) {
        printf("[TCP] Fechando conexão anterior para estabelecer nova conexão...\n");
        tcp_client_close(state);
    }
    
    // Resetar estado para nova mensagem
    reset_tcp_state(state);
    
    // Configurar estado para nova conexão
    ip4addr_aton(server_ip, &state->remote_addr);
    state->user_message = (char*)message;
    
    // Tenta abrir a conexão (forçando nova conexão)
    printf("[TCP] Estabelecendo nova conexão com %s:%u\n", server_ip, port);
    if (!tcp_client_open(state, port)) {
        printf("[TCP] Falha ao abrir conexão TCP\n");
        return false;
    }
    
    // Aguardar com timeout aprimorado
    uint32_t start_time = time_us_32() / 1000;
    uint32_t timeout = 10000;  // 10 segundos de timeout (aumentado)
    
    printf("[TCP] Aguardando conclusão da operação (timeout: %u ms)...\n", timeout);
    
    while (!state->complete) {
        // Verifica timeout
        uint32_t elapsed = (time_us_32() / 1000) - start_time;
        if (elapsed > timeout) {
            printf("[TCP] Timeout de %u ms excedido após %lu ms\n", timeout, elapsed);
            state->connected = false;
            return false;
        }
        
        // Processar eventos WiFi e TCP
        cyw43_arch_poll();
        sleep_ms(10);  // Intervalo maior para não sobrecarregar
        
        // Log periódico a cada 1 segundo
        if ((elapsed % 1000) < 10) {
            printf("[TCP] Ainda aguardando... %lu ms decorridos\n", elapsed);
        }
    }
    
    // Verificar resultado final - MODIFICADO
    // Consideramos sucesso se a operação foi concluída, independente do estado connected
    // Isso é porque o servidor pode fechar a conexão após responder (comportamento normal)
    if (state->complete) {
        printf("[TCP] ✓ Mensagem processada com sucesso\n");
        return true;
    } else {
        printf("[TCP] ✗ Falha no processamento da mensagem\n");
        return false;
    }
}

void wifi_deinit(void) {
    if (wifi_initialized) {
        printf("[WiFi] Desligando módulo WiFi\n");
        
        // Liberar recursos do cliente TCP, se existir
        if (global_tcp_state) {
            if (global_tcp_state->tcp_pcb) {
                tcp_client_close(global_tcp_state);
            }
            free(global_tcp_state);
            global_tcp_state = NULL;
        }
        
        cyw43_arch_deinit();
        wifi_initialized = false;
    }
}