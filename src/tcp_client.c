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
    
    return err;
}

// Callback chamado quando a operação TCP termina
static err_t tcp_result(void *arg, int status) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    if (status == 0) {
        printf("[TCP] Operação concluída com sucesso\n");
    } else {
        printf("[TCP] Falha na operação: %d\n", status);
    }
    
    state->complete = true;
    return tcp_client_close(arg);
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
        printf("[TCP] Falha ao conectar: %d\n", err);
        return tcp_result(arg, err);
    }
    
    state->connected = true;
    printf("[TCP] Conectado ao servidor\n");
    
    // Se temos uma mensagem para enviar, envie-a agora
    if (state->user_message != NULL) {
        cyw43_arch_lwip_begin();
        err = tcp_write(tpcb, state->user_message, strlen(state->user_message), TCP_WRITE_FLAG_COPY);
        cyw43_arch_lwip_end();
        
        if (err != ERR_OK) {
            printf("[TCP] Falha ao enviar dados: %d\n", err);
            return tcp_result(arg, err);
        }
        
        cyw43_arch_lwip_begin();
        err = tcp_output(tpcb);
        cyw43_arch_lwip_end();
        
        if (err != ERR_OK) {
            printf("[TCP] tcp_output falhou: %d\n", err);
            return tcp_result(arg, err);
        }
        
        printf("[TCP] Dados enviados\n");
    } else {
        printf("[TCP] Nenhum dado para enviar\n");
        tcp_result(arg, 0);
    }
    
    return ERR_OK;
}

// Callback chamado periodicamente durante a conexão
static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    printf("[TCP] Tempo limite de conexão\n");
    return tcp_result(arg, -1);
}

// Callback para erros TCP
static void tcp_client_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        printf("[TCP] Erro TCP: %d\n", err);
        tcp_result(arg, err);
    }
}

// Callback chamado quando dados são recebidos
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    
    if (!p) {
        return tcp_result(arg, -1);
    }
    
    // Não precisamos verificar cyw43_arch_lwip_check() por estar em callback do lwIP
    
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
static bool tcp_client_open(TCP_CLIENT_T *state) {
    printf("[TCP] Conectando a %s porta %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        printf("[TCP] Falha ao criar PCB\n");
        return false;
    }
    
    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, 10); // 5 segundos (2*500ms)
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);
    
    state->buffer_len = 0;
    state->sent_len = 0;
    
    // Garantir acesso thread-safe ao lwIP
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);
    cyw43_arch_lwip_end();
    
    return err == ERR_OK;
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
    
    wifi_initialized = true;
    return true;
}

bool tcp_send_message(const char* server_ip, uint16_t port, const char* message) {
    if (!wifi_initialized) {
        printf("[TCP] WiFi não inicializado\n");
        return false;
    }
    
    if (!server_ip || !message) {
        printf("[TCP] Parâmetros inválidos\n");
        return false;
    }
    
    // Criar e inicializar estado do cliente TCP
    TCP_CLIENT_T *state = calloc(1, sizeof(TCP_CLIENT_T));
    if (!state) {
        printf("[TCP] Falha ao alocar memória para o cliente TCP\n");
        return false;
    }
    
    // Converter o IP do servidor para formato binário
    ip4addr_aton(server_ip, &state->remote_addr);
    
    // Definir a mensagem a ser enviada
    state->user_message = (char*)message;
    
    // Abrir conexão TCP
    if (!tcp_client_open(state)) {
        printf("[TCP] Falha ao iniciar conexão\n");
        free(state);
        return false;
    }
    
    // Aguardar até que a operação seja concluída
    while (!state->complete) {
        // Processar eventos WiFi e TCP
        cyw43_arch_poll();
        sleep_ms(1);
    }
    
    // Limpar recursos
    free(state);
    return true;
}

void wifi_deinit(void) {
    if (wifi_initialized) {
        printf("[WiFi] Desligando módulo WiFi\n");
        cyw43_arch_deinit();
        wifi_initialized = false;
    }
}