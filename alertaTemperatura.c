/* =========================================================
 * SISTEMA DE MONITORAMENTO DE TEMPERATURA COM ALERTA
 * 
 * 
 * Descrição: Esse código implementa um sistema de monitoramento 
 * de temperatura com alerta via buzzer e LEDs, além de
 * envio de dados por WiFi para um servidor Python.
 * ========================================================= */

/**
 * @file alertaTemperatura.c
 * @brief Sistema de Monitoramento e Alerta de Temperatura
 * 
 * Este programa implementa um sistema de monitoramento de temperatura
 * usando o sensor interno do RP2040. Quando a temperatura ultrapassa
 * um limite configurado, o sistema aciona um alarme sonoro e visual,
 * além de enviar alertas pela rede.
 * 
 * Recursos utilizados:
 * - Sensor de temperatura interno do RP2040
 * - Display OLED SSD1306 via I2C
 * - Matriz de LEDs WS2818B via PIO
 * - Buzzer via PWM
 * - Comunicação WiFi/TCP para envio de alertas
 */

/* ========== INCLUDES ========== */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// módulos que estão em diretórios organizados
#include "display/oled_display.h"
#include "temperature/temperature_sensor.h"
#include "buzzer/buzzer_control.h"
#include "network/tcp_alerts.h"
#include "network/tcp_client.h"
#include "leds/ledsArray.h"
#include "oled/ssd1306.h"

/* ========== DEFINES - configurações que posso mexer facilmente ========== */
// Pinos - Defini os pinos que vou usar no projeto
#define I2C_SDA 14             // Pino SDA para o display
#define I2C_SCL 15             // Pino SCL para o display
#define BT_A 05                // Botão para aumentar o limite de temperatura
#define BT_B 06                // Botão para diminuir o limite de temperatura
#define BUZZER_PIN_A 21        // Pino do buzzer
#define BUZZER_PIN_B 10        //Pino do buzzer
#define BUZZER_FREQUENCY 1200  // Frequência do buzzer em Hz - som irritante o suficiente kkkk

// Parâmetros do sistema - ajustes gerais
#define TEMP_LIMITE 25.0f      // Temperatura limite inicial (posso mudar pelos botões depois)
#define DELTA_HISTERESE 2.0f   // Margem de histerese pra evitar ficar ligando/desligando direto

// Rede WiFi e TCP
#define WIFI_SSID "Tomada preguicosa"  
#define WIFI_PASSWORD "cachorro123"    
#define WIFI_TIMEOUT_MS 10000          // Tempo de espera pra conexão wifi
#define SERVER_IP "192.168.3.6"        
#define TCP_PORT 5001                  // Porta TCP do servidor
#define TCP_INTERVAL_MS 1000           // Intervalo entre envios TCP - está com taxa de envio de 1 segundo

/* ========== VARIÁVEIS GLOBAIS ========== */
bool alerta_ativo = false;             // Indica se o alarme está tocando ou não
int alert_count = 0;                   // Conta quantas vezes o alarme disparou
float temp_limite_config = TEMP_LIMITE; // Limite que pode ser ajustado pelos botões
bool wifi_conectado = false;           // Status da conexão wifi
uint32_t ultimo_envio_tcp = 0;         // Marca quando foi o último envio de dados
uint32_t tempo_loop = 0;               // Controla a velocidade do loop

/* ========== FUNÇÃO PRINCIPAL - toda a lógica está aqui ========== */
int main() {
    // === INICIALIZAÇÃO BÁSICA ===
    stdio_init_all();             
    sleep_ms(2000);                    
    
    printf("\n\n===== Sistema de Monitoramento de Temperatura com WiFi =====\n\n");
    printf("[TCP] Vou enviar dados para %s na porta %d\n", SERVER_IP, TCP_PORT);
    
    // === INICIALIZAÇÃO DOS COMPONENTES ===
    // Sensor de temperatura (usa o ADC interno do Pico)
    init_temperature_sensor();
    printf("Sensor de temperatura pronto\n");
    
    // Botões para ajuste do limite (com resistor de pull-up interno)
    gpio_init(BT_A);
    gpio_set_dir(BT_A, GPIO_IN);
    gpio_pull_up(BT_A);
    gpio_init(BT_B);
    gpio_set_dir(BT_B, GPIO_IN);
    gpio_pull_up(BT_B);
    printf("Botões prontos\n");
    
    // Display OLED via I2C
    struct render_area frame_area;
    uint8_t *ssd = initialize_oled_display(I2C_SDA, I2C_SCL, &frame_area); //ponteiro para o buffer
    if (!ssd) {
        printf("Erro ao inicializar o display OLED\n");
        return -1;
    }
    printf("Display OLED pronto\n");
    
    // Matriz de LEDs (biblioteca ledsArray)
    npInit(LED_PIN);
    printf("LEDs prontos\n");
    
    // Buzzer (usando PWM)
    pwm_init_buzzer(BUZZER_PIN_A, BUZZER_FREQUENCY);
    pwm_init_buzzer(BUZZER_PIN_B, BUZZER_FREQUENCY);
    printf("Buzzer pronto\n");
    
    // === CONEXÃO WIFI ===
    // Mostra mensagem inicial no display
    char *texto_wifi[] = { "Iniciando...", "Conectando", "WiFi..." };
    display_text(ssd, &frame_area, texto_wifi, 3, 5, 0, 16);
    
    // Tenta conectar ao WiFi
    wifi_conectado = wifi_init_and_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT_MS);
    
    if (wifi_conectado) {
        //pisca LEDs verdes
        for (int i = 0; i < LED_COUNT; i++) {
            npSetLED(i, 0, 30, 0);
        }
        npWrite();
        
        char *texto_wifi_ok[] = { "WiFi Conectado!", "Estabilizando", "conexao..." };
        display_text(ssd, &frame_area, texto_wifi_ok, 3, 5, 0, 16);
        
        // pausa grande depois do wifi porque senão algumas travadas acontecem - ainda não descobri o motivo extao
        printf("WiFi conectado! Aguardando %d ms para estabilizar...\n", 5000);
        sleep_ms(5000);
    } else {
        // Falhou! Pisca LEDs amarelos
        for (int i = 0; i < LED_COUNT; i++) {
            npSetLED(i, 30, 30, 0);
        }
        npWrite();
        
        char *texto_wifi_erro[] = { "Erro WiFi", "Continuando", "modo offline..." };
        display_text(ssd, &frame_area, texto_wifi_erro, 3, 5, 0, 16);
        sleep_ms(2000);
    }
    
    // Apaga os LEDs após o status de inicialização
    for (int i = 0; i < LED_COUNT; i++) {
        npSetLED(i, 0, 0, 0);
    }
    npWrite();
    
    // Inicializa contadores de tempo
    ultimo_envio_tcp = time_us_32() / 1000; // variavel que controla o tempo de envio dos dados
    tempo_loop = time_us_32() / 1000; // variavel que controla o tempo do loop
    int contador_ciclos = 0; 
    
    // === LOOP PRINCIPAL  ===
    printf("Sistema pronto.\n");
    
    while (true) {
        // --- CONTROLE DE TIMING DO LOOP ---
        uint32_t agora = time_us_32() / 1000;
        uint32_t tempo_decorrido = agora - tempo_loop; // diferença entre o tempo atual e o tempo do último loop para calcular o tempo decorrido
        // Se o tempo decorrido for maior que 300ms, atualiza o tempo do loop
        
        //Tempo de ciclo para 300ms
        if (tempo_decorrido < 300) {
            // Em vez de um sleep bloqueante, uso sleep pequenos pra manter o WiFi funcionando
            uint32_t tempo_restante = 300 - tempo_decorrido;
            uint32_t inicio_espera = time_us_32() / 1000;
            
            while ((time_us_32() / 1000) - inicio_espera < tempo_restante) {
                if (wifi_conectado) {
                    cyw43_arch_poll(); // Processa eventos WiFi durante a espera
                }
                sleep_ms(10); // Pequenos intervalos pra manter tudo respondendo
            }
            
            agora = time_us_32() / 1000; // Atualiza o tempo atual
        }
        
        tempo_loop = agora;
        contador_ciclos++; //incrementa o contador de ciclos
        
        // --- LEITURA DA TEMPERATURA ---
        // Usando 100 amostras de temperatura para aumentar a precisão de medição de temperatura
        float temperatura = leitura_temp_precisa(100);
        
        // --- ATUALIZAÇÃO DO DISPLAY ---
        if (contador_ciclos % 2 == 0) {
            // Display atualiza a cada dois ciclos para não ficar piscando
            char temp_str[20];
            // Usando %+6.2f garante espaço para o sinal e alinhamento consistente
            sprintf(temp_str, "Temp:%+6.2f C", temperatura);
            
            char alert_str[20];
            sprintf(alert_str, "Alertas: %d", alert_count);
            
            char limit_str[20];
            // Mesmo formato para o limite
            sprintf(limit_str, "Limite:%+5.1f C", temp_limite_config);
            
            char wifi_str[20];
            sprintf(wifi_str, "WiFi: %s", wifi_conectado ? "ON" : "OFF");
            
            // Atualiza o display OLED com as leituras
            memset(ssd, 0, ssd1306_buffer_length);
            char *temp_text[] = { temp_str, alert_str, limit_str, wifi_str };
            display_text(ssd, &frame_area, temp_text, 4, 5, 0, 16);
            
            // Também atualiza o formato no printf do terminal
            printf("Temp: %+.2f C | Limite: %+.1f C | Alerta: %s | Alertas: %d\n", 
                   temperatura, temp_limite_config, 
                   alerta_ativo ? "Ativo" : "Inativo", alert_count);
        }
        
        // --- AJUSTE DO LIMITE VIA BOTÕES ---
        // Botão A aumenta o limite
        if (!gpio_get(BT_A)) {
            temp_limite_config += 1.0f;
            printf("Limite aumentado: %.1f C\n", temp_limite_config);
            sleep_ms(200);  // Debounce 
        }
        // Botão B diminui o limite
        if (!gpio_get(BT_B)) {
            temp_limite_config -= 1.0f;
            printf("Limite diminuído: %.1f C\n", temp_limite_config);
            sleep_ms(200);  // Debounce
        }
        
        // --- CONTROLE DE ALERTA COM HISTERESE ---
        // Ativa o alerta quando temperatura passa do limite
        if (temperatura >= temp_limite_config && !alerta_ativo) {
            alerta_ativo = true;
            alert_count++;
            
            // Leds e buzzers são ativados par ao alerta
            for (int i = 0; i < LED_COUNT; i++) {
                npSetLED(i, 30, 0, 0); // LEDs vermelhos
            }
            npWrite();
            start_beep(BUZZER_PIN_A);
            start_beep(BUZZER_PIN_B);
            
            printf("ALERTA! Temperatura %.2f acima do limite %.1f\n", temperatura, temp_limite_config);
            
            // Envio imediato de alerta via wifi se conectado
            if (wifi_conectado) {
                printf("[TCP] Enviando alerta para %s:%d...\n", SERVER_IP, TCP_PORT);
                bool sucesso = enviar_temperatura_tcp(temperatura, temp_limite_config, alerta_ativo, alert_count,SERVER_IP, TCP_PORT);
                if (sucesso) {
                    printf("[TCP] Alerta enviado com sucesso!\n");
                } else {
                    printf("[TCP] Falha no envio do alerta\n");
                }
                ultimo_envio_tcp = agora;
            }
        } 
        // Desativa o alerta quando temperatura cai abaixo do limite - HISTERESE
        // A histerese evita que fique ligando/desligando com pequenas oscilações
        else if (temperatura < (temp_limite_config - DELTA_HISTERESE) && alerta_ativo) {
            alerta_ativo = false;
            
            // Desliga LEDs e buzzer
            for (int i = 0; i < LED_COUNT; i++) {
                npSetLED(i, 0, 0, 0); // Apaga LEDs
            }
            npWrite();
            stop_beep(BUZZER_PIN_A);
            stop_beep(BUZZER_PIN_B);
            
            printf("Temperatura %.2f voltou ao normal (abaixo de %.1f)\n", 
                  temperatura, temp_limite_config);
            
            // Envia notificação de normalização via wifi se conectado
            if (wifi_conectado) {
                printf("[TCP] Enviando normalização para %s:%d...\n", SERVER_IP, TCP_PORT);
                bool sucesso = enviar_temperatura_tcp(temperatura, temp_limite_config, alerta_ativo, alert_count,SERVER_IP, TCP_PORT);
                if (sucesso) {
                    printf("[TCP] Normalização enviada com sucesso\n");
                } else {
                    printf("[TCP] Falha no envio de normalização\n");
                }
                ultimo_envio_tcp = agora;
            }
        }
        
        // --- ENVIO PERIÓDICO DE DADOS VIA TCP ---
        // Envia a cada TCP_INTERVAL_MS 
        if (wifi_conectado && (agora - ultimo_envio_tcp >= TCP_INTERVAL_MS)) { // se wifi conectado e já passou o tempo de envio

            printf("[TCP] Enviando dados para %s:%d...\n", SERVER_IP, TCP_PORT);
            bool sucesso = enviar_temperatura_tcp(temperatura, temp_limite_config,alerta_ativo, alert_count,SERVER_IP, TCP_PORT);
            
            if (sucesso) {
                printf("[TCP] Dados enviados com sucesso #%d\n", contador_ciclos);
            } else {
                printf("[TCP] Falha no envio #%d - verifique a conexão...\n", contador_ciclos);
                
                // A cada 10 falhas, tenta reconectar o WiFi
                // (não faz toda vez pra não ficar travando)
                if (contador_ciclos % 10 == 0) {
                    printf("[WiFi] Tentando reconectar...\n");
                    
                    // Mostra mensagem no display
                    char *texto_reconexao[] = { "Reconectando", "WiFi...", "" };
                    display_text(ssd, &frame_area, texto_reconexao, 3, 5, 0, 16);
                    
                    // Tenta reconectar
                    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 5000) == 0) {
                        wifi_conectado = true;
                        printf("[WiFi] Reconectado.\n");
                    } else {
                        wifi_conectado = false;
                        printf("[WiFi] Falha na reconexão.\n");
                    }
                }
            }
            
            ultimo_envio_tcp = agora;
        }
        
        // --- MANUTENÇÃO DO WIFI ---
        // Processa eventos WiFi no final do loop
        if (wifi_conectado) {
            cyw43_arch_poll(); // essa função em especifico é importante pra manter a conexão estável
        }
    }
    
    if (wifi_conectado) {
        wifi_deinit(); // Desconcecta o wifi
    }
    
    return 0;
}