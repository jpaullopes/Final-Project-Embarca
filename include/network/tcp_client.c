/**
 * @file tcp_client.c
 * @brief Implementação de comunicação TCP via WiFi para Raspberry Pi Pico W
 *
 * Esse módulo implementa a comunicação TCP utilizando lwIP sobre o hardware WiFi do Pico.
 * Ele gerencia a conexão, o envio de mensagens, e reconexões em caso de falhas.
 */

 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "pico/stdlib.h"
 #include "pico/cyw43_arch.h"
 #include "lwip/pbuf.h"
 #include "lwip/tcp.h"
 #include "tcp_client.h"
 
 // Flag de inicialização do WiFi. Indica se o módulo já foi inicializado.
 static bool wifi_initialized = false;
 
 // Estado global da conexão TCP persistente.
 // Mantém o contexto da conexão para possibilitar reuso entre envios.
 static TCP_CLIENT_T* global_tcp_state = NULL;
 
 // Controle de tentativas e tempo para reconexão de TCP.
 static uint32_t reconnect_attempts = 0;
 static absolute_time_t last_reconnect_time;
 static const uint32_t RECONNECT_INTERVAL_MS = 5000; // 5 segundos entre tentativas
 static const uint32_t MAX_RECONNECT_ATTEMPTS = 10;    // Máximo de tentativas de reconexão
 
 /**
  * @brief Callback chamado para fechar a conexão TCP.
  *
  * Esta função é utilizada para limpar a conexão, ajustando callbacks para NULL,
  * tentando fechar a conexão e, em caso de erro, abortando a mesma.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @return err_t Código do resultado da operação.
  */
 static err_t tcp_client_close(void *arg) {
     TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
     err_t err = ERR_OK;
     
     if (state->tcp_pcb != NULL) {
         // Remove argumentos e callbacks associados à conexão
         tcp_arg(state->tcp_pcb, NULL);
         tcp_poll(state->tcp_pcb, NULL, 0);
         tcp_sent(state->tcp_pcb, NULL);
         tcp_recv(state->tcp_pcb, NULL);
         tcp_err(state->tcp_pcb, NULL);
         
         // Tenta fechar a conexão TCP de forma ordenada
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
 
 /**
  * @brief Callback chamado quando a operação TCP termina.
  *
  * Quando o envio termina (ou ocorre erro), essa função é invocada para sinalizar
  * que a operação foi concluída. Reinicia o contador de tentativas em caso de sucesso.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @param status Código de status da operação (0 indica sucesso).
  * @return err_t Código de retorno conforme processamento.
  */
 static err_t tcp_result(void *arg, int status) {
     TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
     
     if (status == 0) {
         printf("[TCP] Operação concluída com sucesso\n");
         reconnect_attempts = 0; // Reinicia a contagem em caso de sucesso
     } else {
         // Se houver erro, pode-se logar o status se necessário.
     }
     
     state->complete = true;
     
     // Se houve erro, a conexão é fechada.
     if (status != 0) {
         return tcp_client_close(arg);
     }
     return ERR_OK;
 }
 
 /**
  * @brief Callback chamado quando dados são enviados via TCP.
  *
  * Essa função é invocada após o envio de dados. Incrementa o contador de bytes enviados.
  * Quando o total enviado atinge o tamanho da mensagem, a operação é considerada concluída.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @param tpcb PCB da conexão TCP.
  * @param len Número de bytes enviados.
  * @return err_t Código de operação.
  */
 static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
     TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
     
     printf("[TCP] Enviados %u bytes\n", len);
     state->sent_len += len;
     
     // Verifica se todos os bytes foram enviados
     if (state->user_message != NULL && state->sent_len >= strlen(state->user_message)) {
         tcp_result(arg, 0);
     }
     
     return ERR_OK;
 }
 
 /**
  * @brief Callback chamado quando a conexão TCP é estabelecida.
  *
  * Após a conexão ser estabelecida, esta função configura o estado como conectado
  * e, se houver mensagem pendente, inicia o envio da mesma.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @param tpcb PCB da conexão TCP.
  * @param err Código de erro recebido durante a conexão.
  * @return err_t Código de resultado.
  */
 static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
     TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
     
     if (err != ERR_OK) {
         printf("[TCP] ERRO CRÍTICO: Falha ao conectar: %d\n", err);
         return tcp_result(arg, err);
     }
     
     state->connected = true;
     printf("[TCP] ✓ Conectado com sucesso ao servidor %s:%d\n", 
            ip4addr_ntoa(&state->remote_addr), tpcb->remote_port);
     
     // Se há mensagem para enviar, inicia o envio
     if (state->user_message != NULL) {
         // Exibe parte da mensagem para log (limitada a 40 caracteres)
         char short_msg[41];
         strncpy(short_msg, state->user_message, 40);
         short_msg[40] = '\0';
         printf("[TCP] Enviando: '%s%s'\n", short_msg, 
                strlen(state->user_message) > 40 ? "..." : "");
         
         // Envio dos dados utilizando funções seguras do lwIP
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
 
 /**
  * @brief Callback chamado periodicamente durante a conexão TCP.
  *
  * Essa função é chamada para manter o estado da conexão e não é considerada um erro.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @param tpcb PCB da conexão TCP.
  * @return err_t Código de operação.
  */
 static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
     printf("[TCP] Tempo limite de poll atingido\n");
     return ERR_OK;
 }
 
 /**
  * @brief Callback para tratamento de erros na conexão TCP.
  *
  * Essa função é chamada quando ocorre um erro na conexão.
  * Se o erro não for de abort, marca a conexão como desconectada e completa.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @param err Código de erro ocorrido.
  */
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
 
 /**
  * @brief Callback chamado quando dados são recebidos.
  *
  * Essa função processa os dados recebidos do servidor, copiando-os para o buffer
  * e informando o lwIP sobre os bytes processados.
  *
  * @param arg Ponteiro para a estrutura do cliente TCP.
  * @param tpcb PCB da conexão TCP.
  * @param p Ponteiro para o pbuf contendo os dados recebidos.
  * @param err Código de erro associado à recepção.
  * @return err_t Código de operação.
  */
 static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
     TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
     
     if (!p) {
         // Se p for NULL, significa que o servidor fechou a conexão.
         printf("[TCP] Conexão fechada pelo servidor (comportamento normal)\n");
         
         state->connected = false;
         
         // Se houve dados no buffer, considera a operação bem-sucedida.
         if (state->buffer_len > 0) {
             printf("[TCP] ✓ Comunicação completa e bem-sucedida\n");
             state->complete = true;
             return ERR_OK;
         } else {
             printf("[TCP] ✓ Dados enviados e conexão finalizada normalmente\n");
             state->complete = true;
             return ERR_OK;
         }
     }
     
     // Se há dados no pbuf, processa-os.
     if (p->tot_len > 0) {
         printf("[TCP] Recebidos %d bytes\n", p->tot_len);
         
         const uint16_t buffer_left = TCP_BUF_SIZE - state->buffer_len;
         state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len,
                                                p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
         
         // Se houver espaço, adiciona terminador nulo para impressão.
         if (state->buffer_len < TCP_BUF_SIZE) {
             state->buffer[state->buffer_len] = '\0';
             printf("[TCP] Dados: %s\n", state->buffer);
         }
         
         // Informa o lwIP que os dados foram processados.
         tcp_recved(tpcb, p->tot_len);
     }
     
     pbuf_free(p);
     return ERR_OK;
 }
 
 /**
  * @brief Inicializa e abre a conexão TCP.
  *
  * Cria um novo PCB, configura os callbacks e tenta conectar à porta especificada.
  *
  * @param state Ponteiro para a estrutura do cliente TCP.
  * @param port Porta do servidor para conexão.
  * @return true se a conexão foi estabelecida com sucesso, false caso contrário.
  */
 static bool tcp_client_open(TCP_CLIENT_T *state, uint16_t port) {
     printf("[TCP] Conectando a %s porta %u\n", ip4addr_ntoa(&state->remote_addr), port);
     
     state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
     if (!state->tcp_pcb) {
         printf("[TCP] Falha ao criar PCB\n");
         return false;
     }
     
     // Configura os callbacks para a nova conexão
     tcp_arg(state->tcp_pcb, state);
     tcp_poll(state->tcp_pcb, tcp_client_poll, 5);
     tcp_sent(state->tcp_pcb, tcp_client_sent);
     tcp_recv(state->tcp_pcb, tcp_client_recv);
     tcp_err(state->tcp_pcb, tcp_client_err);
     
     state->buffer_len = 0;
     state->sent_len = 0;
     state->complete = false;
     
     // Protege a operação com acesso thread-safe ao lwIP
     cyw43_arch_lwip_begin();
     err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, port, tcp_client_connected);
     cyw43_arch_lwip_end();
     
     return err == ERR_OK;
 }
 
 /**
  * @brief Recupera ou cria o estado global TCP.
  *
  * Se ainda não existe, aloca memória para o estado global.
  *
  * @return Ponteiro para a estrutura TCP_CLIENT_T.
  */
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
 
 /**
  * @brief Reseta o estado do cliente TCP antes de iniciar uma nova operação.
  *
  * Zera os contadores e indica que a operação não está completa.
  *
  * @param state Ponteiro para a estrutura do cliente TCP.
  */
 static void reset_tcp_state(TCP_CLIENT_T *state) {
     if (state) {
         state->buffer_len = 0;
         state->sent_len = 0;
         state->complete = false;
         state->user_message = NULL;
     }
 }
 
 /**
  * @brief Verifica e tenta reconectar caso a conexão TCP esteja inativa.
  *
  * Se a conexão não estiver ativa, essa função tenta reconectar 
  * respeitando um intervalo entre tentativas e um número máximo de tentativas.
  *
  * @param state Ponteiro para a estrutura do cliente TCP.
  * @param server_ip Endereço IP do servidor.
  * @param port Porta do servidor.
  * @return true se a reconexão foi bem-sucedida, false caso contrário.
  */
 static bool ensure_tcp_connection(TCP_CLIENT_T *state, const char* server_ip, uint16_t port) {
     if (state->connected && state->tcp_pcb != NULL) {
         return true;
     }
     
     absolute_time_t now = get_absolute_time();
     if (!is_nil_time(last_reconnect_time) && 
         absolute_time_diff_us(last_reconnect_time, now) < RECONNECT_INTERVAL_MS * 1000) {
         return false;
     }
     
     last_reconnect_time = now;
     reconnect_attempts++;
     
     if (reconnect_attempts > MAX_RECONNECT_ATTEMPTS) {
         printf("[TCP] Número máximo de tentativas de reconexão atingido (%lu)\n", reconnect_attempts);
         return false;
     }
     
     printf("[TCP] Tentativa de reconexão %lu de %u\n", reconnect_attempts, MAX_RECONNECT_ATTEMPTS);
     
     if (state->tcp_pcb != NULL) {
         tcp_client_close(state);
     }
     
     ip4addr_aton(server_ip, &state->remote_addr);
     
     return tcp_client_open(state, port);
 }
 
 /**
  * @brief Inicializa o WiFi e conecta à rede usando parâmetros fornecidos.
  *
  * Inicializa o módulo WiFi e tenta conectar com um timeout especificado.
  *
  * @param ssid Nome da rede WiFi.
  * @param password Senha da rede WiFi.
  * @param timeout_ms Tempo máximo para a conexão (em ms).
  * @return true se conectado com sucesso, false caso contrário.
  */
 bool wifi_init_and_connect(const char* ssid, const char* password, uint32_t timeout_ms) {
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
     
     reconnect_attempts = 0;
     last_reconnect_time = nil_time;
     
     wifi_initialized = true;
     return true;
 }
 
 /**
  * @brief Envia uma mensagem via TCP para um servidor remoto.
  *
  * Configura e abre a conexão TCP, envia a mensagem, aguarda a conclusão da operação
  * com timeout e retorna o status do envio.
  *
  * @param server_ip Endereço IP do servidor.
  * @param port Porta do servidor.
  * @param message Mensagem a ser enviada.
  * @return true se a mensagem foi enviada com sucesso, false caso contrário.
  */
 bool tcp_send_message(const char* server_ip, uint16_t port, const char* message) {
     printf("[TCP] Tentando enviar para %s:%u: '%s'\n", server_ip, port, message);
     
     if (!wifi_initialized) {
         printf("[TCP] WiFi não inicializado. Inicialize o WiFi primeiro.\n");
         return false;
     }
     
     if (!server_ip || !message) {
         printf("[TCP] Parâmetros inválidos (IP ou mensagem nulos)\n");
         return false;
     }
 
     ip_addr_t test_addr;
     if (ipaddr_aton(server_ip, &test_addr) == 0) {
         printf("[TCP] ERRO: Formato de IP inválido: %s\n", server_ip);
         return false;
     } else {
         printf("[TCP] IP validado: %s\n", ip4addr_ntoa(&test_addr));
     }
     
     TCP_CLIENT_T *state = get_tcp_state();
     if (!state) {
         printf("[TCP] Falha ao obter estado TCP. Memória insuficiente?\n");
         return false;
     }
 
     if (state->tcp_pcb != NULL) {
         printf("[TCP] Fechando conexão anterior para estabelecer nova conexão...\n");
         tcp_client_close(state);
     }
     
     reset_tcp_state(state);
     
     ip4addr_aton(server_ip, &state->remote_addr);
     state->user_message = (char*)message;
     
     printf("[TCP] Estabelecendo nova conexão com %s:%u\n", server_ip, port);
     if (!tcp_client_open(state, port)) {
         printf("[TCP] Falha ao abrir conexão TCP\n");
         return false;
     }
     
     uint32_t start_time = time_us_32() / 1000;
     uint32_t timeout = 10000;  // Timeout de 10 segundos
     
     printf("[TCP] Aguardando conclusão da operação (timeout: %u ms)...\n", timeout);
     
     while (!state->complete) {
         uint32_t elapsed = (time_us_32() / 1000) - start_time;
         if (elapsed > timeout) {
             printf("[TCP] Timeout de %u ms excedido após %lu ms\n", timeout, elapsed);
             state->connected = false;
             return false;
         }
         
         cyw43_arch_poll();
         sleep_ms(10);
         
         if ((elapsed % 1000) < 10) {
             printf("[TCP] Ainda aguardando... %lu ms decorridos\n", elapsed);
         }
     }
     
     if (state->complete) {
         printf("[TCP] ✓ Mensagem processada com sucesso\n");
         return true;
     } else {
         printf("[TCP] ✗ Falha no processamento da mensagem\n");
         return false;
     }
 }
 
 /**
  * @brief Desliga o módulo WiFi e libera recursos associados.
  *
  * Se o WiFi estiver inicializado, libera a memória do estado TCP global e
  * desativa o hardware WiFi.
  */
 void wifi_deinit(void) {
     if (wifi_initialized) {
         printf("[WiFi] Desligando módulo WiFi\n");
         
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