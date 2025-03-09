/**
 * @file pico_wifi_lib.c
 * @brief Implementação da biblioteca para gerenciar conexão WiFi e comunicação TCP no Raspberry Pi Pico W
 */

#include "pico_wifi_lib.h"

// Variáveis estáticas para controle de estado
static bool wifi_initialized = false;
static bool wifi_connected = false;

// Protótipos de funções de callback internas
static void tcp_error_callback(void *arg, err_t err);
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);

/**
 * @brief Inicializa o módulo WiFi
 * 
 * @return true se sucesso, false caso contrário
 */
bool wifi_init(void) {
    // Evitar reinicialização se já inicializado
    if (wifi_initialized) {
        return true;
    }
    
    // Inicializar a arquitetura CYW43
    if (cyw43_arch_init()) {
        printf("[WiFi] Falha ao inicializar cyw43_arch\n");
        return false;
    }
    
    // Habilitar o modo estação (cliente)
    cyw43_arch_enable_sta_mode();
    
    wifi_initialized = true;
    printf("[WiFi] Módulo WiFi inicializado com sucesso\n");
    return true;
}

/**
 * @brief Conecta à rede WiFi
 * 
 * @param ssid Nome da rede WiFi
 * @param password Senha da rede WiFi
 * @param timeout_ms Tempo limite para conexão em milissegundos
 * @return true se sucesso, false caso contrário
 */
bool wifi_connect(const char* ssid, const char* password, uint32_t timeout_ms) {
    // Verificar se o WiFi foi inicializado
    if (!wifi_initialized) {
        printf("[WiFi] Erro: Módulo WiFi não inicializado\n");
        return false;
    }
    
    // Verificar parâmetros
    if (!ssid || strlen(ssid) == 0) {
        printf("[WiFi] Erro: SSID inválido\n");
        return false;
    }
    
    printf("[WiFi] Conectando à rede '%s'...\n", ssid);
    
    // Tentar conexão à rede WiFi
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, timeout_ms)) {
        printf("[WiFi] Falha ao conectar à rede WiFi\n");
        return false;
    }
    
    wifi_connected = true;
    printf("[WiFi] Conectado com sucesso à rede '%s'\n", ssid);
    
    // Exibir endereço IP atribuído
    char ip_buffer[16];
    wifi_get_ip(ip_buffer, sizeof(ip_buffer));
    printf("[WiFi] Endereço IP: %s\n", ip_buffer);
    
    return true;
}

/**
 * @brief Retorna o endereço IP atribuído ao Pico W
 * 
 * @param ip_buffer Buffer para armazenar o endereço IP como string
 * @param buffer_size Tamanho do buffer
 */
void wifi_get_ip(char* ip_buffer, size_t buffer_size) {
    if (!ip_buffer || buffer_size < 8) {
        return;
    }
    
    // Obter o endereço IP da interface de rede padrão
    const char* ip_str = ip4addr_ntoa(netif_ip4_addr(netif_default));
    
    if (ip_str) {
        strncpy(ip_buffer, ip_str, buffer_size - 1);
        ip_buffer[buffer_size - 1] = '\0';
    } else {
        strncpy(ip_buffer, "0.0.0.0", buffer_size - 1);
        ip_buffer[buffer_size - 1] = '\0';
    }
}

/**
 * @brief Verifica se o WiFi está conectado à rede
 * 
 * @return true se conectado, false caso contrário
 */
bool wifi_is_connected(void) {
    return wifi_connected;
}

/**
 * @brief Desconecta e desliga o módulo WiFi
 */
void wifi_deinit(void) {
    if (!wifi_initialized) {
        return;
    }
    
    // Desconectar do Wi-Fi se estiver conectado
    if (wifi_connected) {
        printf("[WiFi] Desconectando da rede WiFi\n");
        cyw43_arch_disable_sta_mode();
        wifi_connected = false;
    }
    
    // Desligar o módulo WiFi
    printf("[WiFi] Desligando módulo WiFi\n");
    cyw43_arch_deinit();
    wifi_initialized = false;
}

/**
 * @brief Callback chamado quando ocorre um erro na conexão TCP
 * 
 * @param arg Argumento do usuário (ponteiro para wifi_tcp_client_t)
 * @param err Código de erro
 */
static void tcp_error_callback(void *arg, err_t err) {
    wifi_tcp_client_t *client = (wifi_tcp_client_t*)arg;
    
    // Converter códigos de erro para strings mais amigáveis
    const char* err_str;
    switch (err) {
        case ERR_ABRT: err_str = "conexão abortada"; break;
        case ERR_RST: err_str = "conexão resetada"; break;
        case ERR_CLSD: err_str = "conexão fechada"; break;
        case ERR_CONN: err_str = "não conectado"; break;
        case ERR_TIMEOUT: err_str = "timeout"; break;
        default: err_str = "desconhecido"; break;
    }
    
    printf("[WiFi TCP] Erro na conexão: %d (%s)\n", err, err_str);
    
    // O PCB já foi liberado pelo lwIP
    client->pcb = NULL;
    client->complete = true;
    client->connected = false;
}

/**
 * @brief Callback chamado quando dados são recebidos
 * 
 * @param arg Argumento do usuário (ponteiro para wifi_tcp_client_t)
 * @param tpcb Bloco de controle do protocolo TCP
 * @param p Pacote recebido (NULL se a conexão foi fechada)
 * @param err Código de erro
 * @return ERR_OK se processado com sucesso
 */
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    wifi_tcp_client_t *client = (wifi_tcp_client_t*)arg;
    
    if (!p) {
        // Conexão fechada pelo remoto
        printf("[WiFi TCP] Conexão fechada pelo servidor\n");
        client->complete = true;
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    // Processar os dados recebidos
    if (p->tot_len > 0) {
        // Copiar dados do pbuf para o buffer do cliente
        int recv_len = pbuf_copy_partial(p, client->buffer, sizeof(client->buffer) - 1, 0);
        client->buffer[recv_len] = '\0';
        printf("[WiFi TCP] Resposta recebida: %s\n", client->buffer);
        
        // Informar ao lwIP que os dados foram processados
        tcp_recved(tpcb, p->tot_len);
    }
    
    // Liberar o pacote
    pbuf_free(p);
    
    // Fechar a conexão após receber os dados
    tcp_close(tpcb);
    client->complete = true;
    
    return ERR_OK;
}

/**
 * @brief Callback chamado quando os dados são enviados
 * 
 * @param arg Argumento do usuário
 * @param tpcb Bloco de controle do protocolo TCP
 * @param len Quantidade de bytes enviados
 * @return ERR_OK se processado com sucesso
 */
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("[WiFi TCP] Dados enviados com sucesso (%u bytes)\n", len);
    return ERR_OK;
}

/**
 * @brief Callback chamado quando a conexão TCP é estabelecida
 * 
 * @param arg Argumento do usuário (ponteiro para wifi_tcp_client_t)
 * @param tpcb Bloco de controle do protocolo TCP
 * @param err Código de erro
 * @return ERR_OK se processado com sucesso
 */
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    wifi_tcp_client_t *client = (wifi_tcp_client_t*)arg;
    
    if (err != ERR_OK) {
        printf("[WiFi TCP] Erro ao estabelecer conexão: %d\n", err);
        return err;
    }
    
    client->connected = true;
    printf("[WiFi TCP] Conectado ao servidor!\n");
    
    // Configurar callbacks para esta conexão
    tcp_recv(tpcb, tcp_recv_callback);
    tcp_sent(tpcb, tcp_sent_callback);
    
    // Verificar se há dados personalizados para enviar
    const char *data_to_send = client->user_data;
    
    if (data_to_send && strlen(data_to_send) > 0) {
        printf("[WiFi TCP] Enviando: %s\n", data_to_send);
        
        // Enviar os dados
        err_t send_err = tcp_write(tpcb, data_to_send, strlen(data_to_send), TCP_WRITE_FLAG_COPY);
        if (send_err != ERR_OK) {
            printf("[WiFi TCP] Erro ao escrever dados: %d\n", send_err);
            return send_err;
        }
        
        send_err = tcp_output(tpcb);
        if (send_err != ERR_OK) {
            printf("[WiFi TCP] Erro ao enviar pacote: %d\n", send_err);
            return send_err;
        }
    }
    
    return ERR_OK;
}

/**
 * @brief Envia dados para um servidor TCP
 * 
 * @param server_ip Endereço IP do servidor
 * @param server_port Porta do servidor
 * @param data_type Tipo de dados a serem enviados
 * @param data Dados a serem enviados (para DATA_TYPE_CUSTOM)
 * @param data_len Tamanho dos dados (para DATA_TYPE_CUSTOM)
 * @return true se envio foi bem sucedido, false caso contrário
 */
bool wifi_send_data(const char* server_ip, uint16_t server_port, wifi_data_type_t data_type, 
                  const char* data, size_t data_len) {
    // Verificar estado do WiFi
    if (!wifi_initialized || !wifi_connected) {
        printf("[WiFi TCP] Erro: WiFi não inicializado ou não conectado\n");
        return false;
    }
    
    // Verificar parâmetros
    if (!server_ip || strlen(server_ip) == 0) {
        printf("[WiFi TCP] Erro: endereço IP do servidor inválido\n");
        return false;
    }
    
    // Preparar os dados a serem enviados
    static char send_buffer[256];
    
    // Inicializar a estrutura do cliente TCP
    wifi_tcp_client_t client = {0};
    
    if (data_type == DATA_TYPE_TEMP_HUMIDITY) {
        // Gerar dados simulados de temperatura e umidade
        float temp = 25.5f + ((float)rand() / (float)RAND_MAX) * 5.0f;
        float hum = 60.0f + ((float)rand() / (float)RAND_MAX) * 15.0f;
        
        // Formatar dados no formato similar a JSON
        snprintf(send_buffer, sizeof(send_buffer), 
                 "{'device': 'pico_w', 'temperature': %.2f, 'humidity': %.2f}",
                 temp, hum);
                 
        client.user_data = send_buffer;
    } 
    else if (data_type == DATA_TYPE_CUSTOM) {
        // Verificar dados customizados
        if (!data || data_len == 0 || data_len >= sizeof(send_buffer)) {
            printf("[WiFi TCP] Erro: dados inválidos para envio\n");
            return false;
        }
        
        // Copiar os dados personalizados para o buffer
        memcpy(send_buffer, data, data_len);
        send_buffer[data_len] = '\0';
        
        client.user_data = send_buffer;
    }
    
    // Converter o IP do servidor para formato binário
    ip_addr_t server_addr;
    if (!ipaddr_aton(server_ip, &server_addr)) {
        printf("[WiFi TCP] Erro: endereço IP inválido\n");
        return false;
    }
    
    // Criar um novo PCB TCP
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("[WiFi TCP] Erro: falha ao criar TCP PCB\n");
        return false;
    }
    
    // Armazenar o PCB no estado do cliente
    client.pcb = pcb;
    
    // Configurar callbacks
    tcp_arg(pcb, &client);
    tcp_err(pcb, tcp_error_callback);
    
    printf("[WiFi TCP] Conectando ao servidor %s na porta %u...\n", server_ip, server_port);
    
    // Iniciar a conexão
    err_t connect_err = tcp_connect(pcb, &server_addr, server_port, tcp_connected_callback);
    if (connect_err != ERR_OK) {
        printf("[WiFi TCP] Erro ao iniciar conexão: %d\n", connect_err);
        tcp_close(pcb);
        return false;
    }
    
    // Aguardar até que a transação seja concluída
    while (!client.complete) {
        // Permitir que o lwIP processe pacotes
        cyw43_arch_poll();
        sleep_ms(10);
    }
    
    printf("[WiFi TCP] Ciclo de comunicação TCP concluído\n");
    return true;
}