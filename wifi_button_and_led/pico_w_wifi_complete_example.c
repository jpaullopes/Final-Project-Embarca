#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "pico/util/datetime.h"
#include "include/ledsArray.h"

#define LED_PIN_SINGLE 12  // Pino do LED único
#define BUTTON1_PIN 5      // Pino do botão 1
#define BUTTON2_PIN 6      // Pino do botão 2
#define WIFI_SSID "Tomada preguicosa"  // Nome da rede Wi-Fi
#define WIFI_PASS "cachorro123"        // Senha da rede Wi-Fi
#define DEBOUNCE_MS 50     // Tempo de debounce para os botões em ms
#define RECONNECT_DELAY_MS 15000  // Tempo entre tentativas de reconexão Wi-Fi
#define MAX_HTTP_RESPONSE_SIZE 2048 // Reduzido para evitar problemas de memória
#define HTTP_PORT 80       // Porta do servidor HTTP

// Protótipos de funções para matriz de LEDs (adicionados para evitar avisos)
void turn_off_all_matrix_leds();
void turn_on_all_matrix_leds();
void set_random_colors_matrix_leds();

// Definição da estrutura ButtonState
typedef struct ButtonState {
    char message[100];
    char timestamp[20];
    bool state;
} ButtonState;

// Estado dos botões inicializados
ButtonState button1 = {"Nenhum evento no botão 1", "00:00:00", false};
ButtonState button2 = {"Nenhum evento no botão 2", "00:00:00", false};
bool led_state = false;    // Estado atual do LED único

// Estado da matriz de LEDs
uint8_t led_matrix_state[LED_COUNT][3] = {0}; // Inicializa com zeros (LEDs desligados)

// Buffer para resposta HTTP
char http_response[MAX_HTTP_RESPONSE_SIZE];

// Flag para verificar se o servidor HTTP está iniciado
static bool http_server_started = false;

// Estado da conexão TCP
typedef struct {
    bool sending;
    bool close_after_send;
} tcp_conn_state_t;

// Função para obter o timestamp baseado no tempo desde boot
void get_timestamp(char *buffer, size_t size) {
    uint32_t ms_since_boot = to_ms_since_boot(get_absolute_time());
    uint32_t seconds = ms_since_boot / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    // Formata como HH:MM:SS
    snprintf(buffer, size, "%02lu:%02lu:%02lu", 
             hours % 24, minutes % 60, seconds % 60);
}

// Função para criar uma página HTML simplificada
void create_simple_html_response() {
    snprintf(http_response, MAX_HTTP_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n"
             "<!DOCTYPE html><html><head>"
             "<title>Pico W</title>"
             "<style>"
             "body{font-family:Arial;margin:20px;}"
             ".btn{display:inline-block;padding:10px;background:#0066cc;color:white;margin:5px;text-decoration:none;border-radius:4px}"
             ".led{width:20px;height:20px;display:inline-block;background:#333;border-radius:50%%;margin:3px}"
             "</style></head><body>"
             "<h1>Pico W - Controle</h1>"
             "<div><h2>LED Onboard</h2>"
             "<a class='btn' href='/led/on'>Ligar</a>"
             "<a class='btn' href='/led/off'>Desligar</a>"
             "<p>Estado: %s</p></div>"
             "<div><h2>Matriz LED</h2>"
             "<a class='btn' href='/led/matrix/all/on'>Todos ligados</a>"
             "<a class='btn' href='/led/matrix/all/off'>Todos desligados</a>"
             "<a class='btn' href='/led/matrix/random'>Cores aleatórias</a>"
             "</div>"
             "<div><h2>Estado Botões</h2>"
             "<p>Botão 1: %s - %s</p>"
             "<p>Botão 2: %s - %s</p>"
             "</div>"
             "</body></html>",
             led_state ? "LIGADO" : "DESLIGADO",
             button1.message, button1.timestamp,
             button2.message, button2.timestamp);
}

// Função para criar resposta JSON para API
void create_json_response() {
    snprintf(http_response, MAX_HTTP_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nConnection: close\r\n\r\n"
             "{\"led_state\":%s,"
             "\"button1\":{\"message\":\"%s\",\"timestamp\":\"%s\",\"state\":%s},"
             "\"button2\":{\"message\":\"%s\",\"timestamp\":\"%s\",\"state\":%s}}",
             led_state ? "true" : "false",
             button1.message, button1.timestamp, button1.state ? "true" : "false",
             button2.message, button2.timestamp, button2.state ? "true" : "false");
}

// Função para enviar resposta TCP com segurança
static err_t tcp_write_safe(struct tcp_pcb *tpcb, const char *data, size_t len) {
    err_t err;
    uint16_t space;
    size_t sent = 0;
    size_t chunk_size;
    
    while (sent < len) {
        space = tcp_sndbuf(tpcb); // Obtém o espaço disponível no buffer
        if (space == 0) {
            // Sem espaço disponível, aguarde a liberação do buffer
            return ERR_MEM;
        }
        
        // Envia no máximo o espaço disponível
        chunk_size = (len - sent) > space ? space : (len - sent);
        err = tcp_write(tpcb, data + sent, chunk_size, TCP_WRITE_FLAG_COPY);
        
        if (err != ERR_OK) {
            return err;
        }
        
        sent += chunk_size;
        
        // Envia os dados imediatamente
        err = tcp_output(tpcb);
        if (err != ERR_OK) {
            return err;
        }
    }
    
    return ERR_OK;
}

// Callback para quando os dados são enviados
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_conn_state_t *conn_state = (tcp_conn_state_t*)arg;
    
    if (conn_state->close_after_send) {
        conn_state->sending = false;
        tcp_close(tpcb);
        free(conn_state); // Libera a memória do estado
    }
    
    return ERR_OK;
}

// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    tcp_conn_state_t *conn_state = (tcp_conn_state_t*)arg;
    
    if (!conn_state) {
        // Criar novo estado de conexão se não existir
        conn_state = (tcp_conn_state_t*)malloc(sizeof(tcp_conn_state_t));
        if (!conn_state) {
            tcp_abort(tpcb);
            return ERR_ABRT;
        }
        conn_state->sending = false;
        conn_state->close_after_send = false;
        tcp_arg(tpcb, conn_state);
    }

    if (err != ERR_OK) {
        printf("Erro no callback HTTP: %d\n", err);
        if (p) pbuf_free(p);
        return err;
    }

    if (p == NULL) {
        // Cliente fechou a conexão
        printf("Cliente fechou conexão\n");
        free(conn_state);
        tcp_arg(tpcb, NULL);
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a requisição HTTP
    char *request = (char *)p->payload;
    bool is_api_request = false;

    printf("Requisição: %.30s\n", request);

    // Pisca o LED onboard para indicar tráfego
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(20);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // Verifica requisições relacionadas à matriz de LEDs
    if (strstr(request, "GET /led/matrix/all/on")) {
        // Liga todos os LEDs
        for (int i = 0; i < LED_COUNT; i++) {
            led_matrix_state[i][0] = 30;
            led_matrix_state[i][1] = 30;
            led_matrix_state[i][2] = 0;
        }
        ligarTodosOsLEDs();
        npWrite();
        printf("Todos LEDs ligados\n");
    } else if (strstr(request, "GET /led/matrix/all/off")) {
        // Desliga todos os LEDs
        for (int i = 0; i < LED_COUNT; i++) {
            led_matrix_state[i][0] = 0;
            led_matrix_state[i][1] = 0;
            led_matrix_state[i][2] = 0;
        }
        npClear();
        npWrite();
        printf("Todos LEDs desligados\n");
    } else if (strstr(request, "GET /led/matrix/random")) {
        // LEDs com cores aleatórias
        for (int i = 0; i < LED_COUNT; i++) {
            led_matrix_state[i][0] = rand() % 10;
            led_matrix_state[i][1] = rand() % 10;
            led_matrix_state[i][2] = rand() % 10;
        }
        ligarTodosLedsCoresAleatorias();
        npWrite();
        printf("Cores aleatórias nos LEDs\n");
    }
    // Requisições para o LED único
    else if (strstr(request, "GET /led/on")) {
        gpio_put(LED_PIN_SINGLE, 1);
        led_state = true;
        printf("LED ligado\n");
    } else if (strstr(request, "GET /led/off")) {
        gpio_put(LED_PIN_SINGLE, 0);
        led_state = false;
        printf("LED desligado\n");
    } else if (strstr(request, "GET /api/")) {
        is_api_request = true;
    }

    // Decide o formato da resposta
    if (is_api_request) {
        create_json_response();
    } else {
        create_simple_html_response();
    }

    // Libera o buffer de requisição, já não é necessário
    pbuf_free(p);

    // Configura para envio e fechamento posterior
    conn_state->sending = true;
    conn_state->close_after_send = true;
    
    // Configura callback de envio
    tcp_sent(tpcb, tcp_sent_callback);
    
    // Envia resposta HTTP
    err_t result = tcp_write_safe(tpcb, http_response, strlen(http_response));
    if (result != ERR_OK) {
        printf("Erro ao enviar resposta: %d\n", result);
        free(conn_state);
        tcp_abort(tpcb);
        return ERR_ABRT;
    }
    
    return ERR_OK;
}

// Callback de erro para conexões TCP
static void tcp_error_callback(void *arg, err_t err) {
    tcp_conn_state_t *conn_state = (tcp_conn_state_t*)arg;
    if (conn_state) {
        free(conn_state);
    }
    printf("Erro TCP: %d\n", err);
}

// Callback para conexões aceitas
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        printf("Erro ao aceitar conexão: %d\n", err);
        return ERR_VAL;
    }

    printf("Conexão aceita\n");
    
    // Cria estado da conexão
    tcp_conn_state_t *conn_state = (tcp_conn_state_t*)malloc(sizeof(tcp_conn_state_t));
    if (!conn_state) {
        tcp_abort(newpcb);
        return ERR_ABRT;
    }
    
    conn_state->sending = false;
    conn_state->close_after_send = false;
    
    // Associa o estado à conexão
    tcp_arg(newpcb, conn_state);
    
    // Configura callbacks
    tcp_recv(newpcb, http_callback);
    tcp_err(newpcb, tcp_error_callback);
    
    // Configura tempos limite - importante para evitar conexões zombies
    tcp_poll(newpcb, NULL, 4); // 4 * 500ms = 2s
    
    return ERR_OK;
}

// Função de setup do servidor TCP
static bool start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("ERRO: Falha ao criar PCB\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ADDR_ANY, HTTP_PORT);
    if (err != ERR_OK) {
        printf("ERRO: Falha ao ligar servidor porta %d (erro %d)\n", HTTP_PORT, err);
        tcp_close(pcb);
        return false;
    }

    pcb = tcp_listen_with_backlog(pcb, 1);
    if (pcb == NULL) {
        printf("ERRO: Falha ao iniciar escuta\n");
        return false;
    }

    tcp_accept(pcb, connection_callback);

    printf("Servidor HTTP iniciado na porta %d\n", HTTP_PORT);
    printf("Acesse: http://%d.%d.%d.%d\n", 
           ip4_addr1(&cyw43_state.netif[0].ip_addr),
           ip4_addr2(&cyw43_state.netif[0].ip_addr),
           ip4_addr3(&cyw43_state.netif[0].ip_addr),
           ip4_addr4(&cyw43_state.netif[0].ip_addr));
    return true;
}

// Função para monitorar o estado dos botões com debounce
void monitor_buttons() {
    static uint32_t last_button1_time = 0;
    static uint32_t last_button2_time = 0;
    static bool button1_last_state = false;
    static bool button2_last_state = false;
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Lê os estados dos botões (com lógica invertida devido ao pull-up)
    bool button1_current = !gpio_get(BUTTON1_PIN);
    bool button2_current = !gpio_get(BUTTON2_PIN);
    
    // Debounce para botão 1
    if (button1_current != button1_last_state && (current_time - last_button1_time) > DEBOUNCE_MS) {
        button1_last_state = button1_current;
        last_button1_time = current_time;
        button1.state = button1_current;
        
        get_timestamp(button1.timestamp, sizeof(button1.timestamp));
        
        if (button1_current) {
            snprintf(button1.message, sizeof(button1.message), "Botão 1 foi pressionado!");
            printf("Botão 1 pressionado em %s\n", button1.timestamp);
            
            // Aciona os LEDs com cores aleatórias quando botão 1 é pressionado
            set_random_colors_matrix_leds();
        } else {
            snprintf(button1.message, sizeof(button1.message), "Botão 1 foi solto!");
            printf("Botão 1 solto em %s\n", button1.timestamp);
        }
    }
    
    // Debounce para botão 2
    if (button2_current != button2_last_state && (current_time - last_button2_time) > DEBOUNCE_MS) {
        button2_last_state = button2_current;
        last_button2_time = current_time;
        button2.state = button2_current;
        
        get_timestamp(button2.timestamp, sizeof(button2.timestamp));
        
        if (button2_current) {
            snprintf(button2.message, sizeof(button2.message), "Botão 2 foi pressionado!");
            printf("Botão 2 pressionado em %s\n", button2.timestamp);
            
            // Liga todos os LEDs quando o botão 2 é pressionado
            turn_on_all_matrix_leds();
        } else {
            snprintf(button2.message, sizeof(button2.message), "Botão 2 foi solto!");
            printf("Botão 2 solto em %s\n", button2.timestamp);
            
            // Desliga todos os LEDs quando o botão 2 é solto
            turn_off_all_matrix_leds();
        }
    }
}

// Tenta conectar ao WiFi com tempo limite
bool connect_to_wifi(uint32_t timeout_ms) {
    printf("Conectando ao Wi-Fi (%s)...\n", WIFI_SSID);
    
    // Inicializa o LED do Pico W
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, timeout_ms)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        return false;
    }
    
    printf("Wi-Fi conectado!\n");
    
    // Pisca o LED para indicar conexão bem-sucedida
    for (int i = 0; i < 3; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(100);
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    
    // Exibe o endereço IP em formato legível
    printf("Endereço IP: %d.%d.%d.%d\n", 
           ip4_addr1(&cyw43_state.netif[0].ip_addr),
           ip4_addr2(&cyw43_state.netif[0].ip_addr),
           ip4_addr3(&cyw43_state.netif[0].ip_addr),
           ip4_addr4(&cyw43_state.netif[0].ip_addr));
    return true;
}

// Verifica o status da conexão Wi-Fi
void check_wifi_status(void) {
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    const char* status_str;
    
    switch (status) {
        case CYW43_LINK_DOWN:
            status_str = "DESCONECTADO";
            break;
        case CYW43_LINK_JOIN:
            status_str = "CONECTANDO";
            break;
        case CYW43_LINK_NOIP:
            status_str = "CONECTADO (SEM IP)";
            break;
        case CYW43_LINK_UP:
            status_str = "CONECTADO";
            break;
        case CYW43_LINK_FAIL:
            status_str = "FALHA";
            break;
        case CYW43_LINK_NONET:
            status_str = "REDE NÃO ENCONTRADA";
            break;
        case CYW43_LINK_BADAUTH:
            status_str = "AUTENTICAÇÃO INVÁLIDA";
            break;
        default:
            status_str = "DESCONHECIDO";
    }
    
    printf("Status do Wi-Fi: %s (%d)\n", status_str, status);
}

// Função para desligar todos os LEDs da matriz
void turn_off_all_matrix_leds() {
    for (int i = 0; i < LED_COUNT; i++) {
        led_matrix_state[i][0] = 0;
        led_matrix_state[i][1] = 0;
        led_matrix_state[i][2] = 0;
    }
    npClear();
    npWrite();
    printf("Todos os LEDs da matriz desligados\n");
}

// Função para ligar todos os LEDs da matriz com amarelo
void turn_on_all_matrix_leds() {
    for (int i = 0; i < LED_COUNT; i++) {
        led_matrix_state[i][0] = 30;
        led_matrix_state[i][1] = 30;
        led_matrix_state[i][2] = 0;
    }
    ligarTodosOsLEDs();
    npWrite();
    printf("Todos os LEDs da matriz ligados\n");
}

// Função para definir cores aleatórias para todos os LEDs
void set_random_colors_matrix_leds() {
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t r = rand() % 10;
        uint8_t g = rand() % 10;
        uint8_t b = rand() % 10;
        led_matrix_state[i][0] = r;
        led_matrix_state[i][1] = g;
        led_matrix_state[i][2] = b;
    }
    ligarTodosLedsCoresAleatorias();
    npWrite();
    printf("Cores aleatórias aplicadas à matriz de LEDs\n");
}

int main() {
    stdio_init_all();
    // Delay para permitir conexão via serial antes de iniciar o programa
    sleep_ms(3000);
    printf("\n\n=== Raspberry Pi Pico W - Servidor HTTP com LEDs e Botões ===\n");

    // Inicializa o Wi-Fi (precisa ser antes da matriz de LEDs)
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }
    
    // Configura o LED onboard
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // Inicializa a matriz de LEDs
    npInit(7); // Inicializa a matriz de LEDs no pino 7
    npClear();
    npWrite();
    printf("Matriz de LEDs inicializada no pino 7\n");

    // Configura e tenta conectar ao Wi-Fi
    cyw43_arch_enable_sta_mode();
    
    // Tenta conectar ao Wi-Fi até conseguir
    while (!connect_to_wifi(10000)) {
        printf("Nova tentativa em %d segundos...\n", RECONNECT_DELAY_MS/1000);
        sleep_ms(RECONNECT_DELAY_MS);
    }
    
    check_wifi_status();

    // Configura o LED único e os botões
    gpio_init(LED_PIN_SINGLE);
    gpio_set_dir(LED_PIN_SINGLE, GPIO_OUT);

    gpio_init(BUTTON1_PIN);
    gpio_set_dir(BUTTON1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON1_PIN);

    gpio_init(BUTTON2_PIN);
    gpio_set_dir(BUTTON2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON2_PIN);

    printf("LED único configurado no pino %d\n", LED_PIN_SINGLE);
    printf("Botões configurados com pull-up nos pinos %d e %d\n", BUTTON1_PIN, BUTTON2_PIN);

    // Inicia o servidor HTTP
    http_server_started = start_http_server();
    if (!http_server_started) {
        printf("ERRO CRÍTICO: Falha ao iniciar servidor HTTP!\n");
        // Continua mesmo assim, podemos tentar reiniciar depois
    }

    // Liga todos os LEDs na inicialização para mostrar que está funcionando
    turn_on_all_matrix_leds();
    sleep_ms(500);
    turn_off_all_matrix_leds();

    // Loop principal
    uint32_t last_wifi_check = 0;
    uint32_t last_server_check = 0;
    
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Verifica conexão Wi-Fi periodicamente
        if (now - last_wifi_check > 60000) { // A cada minuto
            last_wifi_check = now;
            check_wifi_status();
            
            if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
                printf("Conexão Wi-Fi perdida! Tentando reconectar...\n");
                connect_to_wifi(10000);
            }
        }
        
        // Verifica o servidor HTTP e reinicia se necessário
        if (!http_server_started && now - last_server_check > 30000) { // A cada 30 segundos
            last_server_check = now;
            printf("Tentando reiniciar o servidor HTTP...\n");
            http_server_started = start_http_server();
        }
        
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
        monitor_buttons();  // Atualiza o estado dos botões
        sleep_ms(10);       // Pequena pausa para reduzir uso da CPU
    }

    cyw43_arch_deinit();  // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}
