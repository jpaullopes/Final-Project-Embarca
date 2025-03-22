/* ========== INCLUDES ========== */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "include/oled/ssd1306.h"        // Caminho atualizado
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "include/ledsArray.h"           // Atualizado para o caminho correto
#include "hardware/clocks.h"     // para uso de clock
#include "include/tcp_client.h"  // Nossa biblioteca TCP/WiFi

/* ========== DEFINES ========== */
#define I2C_SDA 14
#define I2C_SCL 15
#define TEMP_LIMITE 45.0f       // Limite inicial de temperatura
#define DELTA_HISTERESE 2.0f    // Margem de histerese
#define BT_A 05
#define BT_B 06
#define BUZZER_PIN_A 21
#define BUZZER_PIN_B 10
#define BUZZER_FREQUENCY 1200

// Configurações Wi-Fi e TCP
#define WIFI_SSID "SSID"        // Substitua pela sua rede WiFi
#define WIFI_PASSWORD "PASSWORD"   // Substitua pela sua senha
#define WIFI_TIMEOUT_MS 10000            // Timeout de conexão WiFi: 10 segundos
#define SERVER_IP "SERVERIP"        // IMPORTANTE: Substitua pelo IP real do seu computador
#define TCP_INTERVAL_MS 5000             // Intervalo entre envios TCP (5 segundos)

/* ========== FUNÇÕES AUXILIARES ========== */

// Exibe um array de strings no display OLED, atualizando a área de renderização.
//   ssd: buffer de vídeo do display
//   area: região do display onde o texto será desenhado
//   text: array contendo as mensagens a serem exibidas
//   text_count: quantidade de linhas de texto
//   start_x, start_y: coordenadas iniciais para desenhar o texto
//   line_height: altura entre as linhas do texto
void display_text(uint8_t *ssd, struct render_area *area, char *text[], int text_count, int start_x, int start_y, int line_height) {
    int y = start_y;
    for (int i = 0; i < text_count; i++) {
        ssd1306_draw_string(ssd, start_x, y, text[i]);
        y += line_height;
    }
    render_on_display(ssd, area);
}

// Faz N leituras do canal ADC para obter a média de tensão e converte para temperatura em °C.
//   nro_amostras: número de leituras a serem realizadas
//   Retorna a temperatura calculada em graus Celsius.
float leitura_temp_precisa(int nro_amostras) {
    uint32_t soma = 0;
    for (int i = 0; i < nro_amostras; i++) {
        soma += adc_read();
        sleep_us(50);
    }
    float media = soma / (float)nro_amostras;
    float voltage = media * (3.3f / 4095);
    return 27.0f - ((voltage - 0.706f) / 0.001721f);
}

// Configura o PWM para um pino específico, definindo parâmetros de frequência e duty cycle.
//   gpio: pino de saída
//   slice_num: identificador do slice de PWM
//   channel: canal do PWM
//   duty_cycle: valor de duty cycle para o canal
void configurar_pwm(uint gpio, uint slice_num, uint channel, uint16_t duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    pwm_set_wrap(slice_num, 255);
    pwm_set_chan_level(slice_num, channel, duty_cycle);
    pwm_set_enabled(slice_num, true);
}

// Inicializa o PWM de um buzzer, definindo a frequência de operação.
//   pin: pino onde o buzzer está conectado
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096));
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

// Ativa o buzzer, gerando um som contínuo.
//   pin: pino onde o buzzer está conectado
void start_beep(uint pin) {
    pwm_set_gpio_level(pin, 2048); // Ativa o buzzer
}

// Desativa o buzzer, interrompendo o som.
//   pin: pino onde o buzzer está conectado
void stop_beep(uint pin) {
    pwm_set_gpio_level(pin, 0); // Desativa o buzzer
}

// Envia os dados de temperatura para o servidor via TCP
void enviar_temperatura_tcp(float temperatura, float limite, bool alerta, int num_alertas) {
    // Formata os dados no formato esperado pelo servidor
    char mensagem[256];
    snprintf(mensagem, sizeof(mensagem), 
             "Temperatura: %.2f C | Limite: %.1f C | Alerta: %s | Alertas: %d",
             temperatura, limite,
             alerta ? "Ativo" : "Inativo",
             num_alertas);
    
    // Envia a mensagem via TCP
    bool resultado = tcp_send_message(SERVER_IP, TCP_PORT, mensagem);
    
    if (resultado) {
        printf("[TCP] Mensagem enviada com sucesso!\n");
    } else {
        printf("[TCP] Falha ao enviar mensagem\n");
    }
}

/* ========== VARIÁVEIS GLOBAIS ========== */
bool alerta_ativo = false;
int alert_count = 0;                // Contador de alertas
float temp_limite_config = TEMP_LIMITE; // Limite de temperatura configurável
bool wifi_conectado = false;        // Status da conexão WiFi
uint32_t ultimo_envio_tcp = 0;      // Momento do último envio TCP

/* ========== FUNÇÃO MAIN ========== */
int main() {
    stdio_init_all();
    sleep_ms(2000); // Aguarda inicialização do console

    printf("\n\n===== Sistema de Monitoramento de Temperatura com WiFi =====\n\n");

    // Inicialização do ADC e configuração do sensor de temperatura
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Configuração dos botões para ajuste do limite de temperatura
    gpio_init(BT_A);
    gpio_set_dir(BT_A, GPIO_IN);
    gpio_pull_up(BT_A);
    gpio_init(BT_B);
    gpio_set_dir(BT_B, GPIO_IN);
    gpio_pull_up(BT_B);

    // Inicializa I2C para comunicação com o display OLED
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa e configura o display OLED
    ssd1306_init();
    struct render_area frame_area = {
        .start_column = 0,
        .end_column   = ssd1306_width - 1,
        .start_page   = 0,
        .end_page     = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Inicializa a matriz de LEDs
    npInit(LED_PIN);

    // Inicializa o buzzer nos pinos configurados
    pwm_init_buzzer(BUZZER_PIN_A);
    pwm_init_buzzer(BUZZER_PIN_B);

    // Inicializa o WiFi e conecta à rede
    char *texto_wifi[] = { "Iniciando...", "Conectando", "WiFi..." };
    display_text(ssd, &frame_area, texto_wifi, 3, 5, 0, 16);
    
    wifi_conectado = wifi_init_and_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT_MS);
    
    // Informa status da conexão WiFi
    if (wifi_conectado) {
        char *texto_wifi_ok[] = { "WiFi Conectado", "Enviando dados", "via TCP..." };
        display_text(ssd, &frame_area, texto_wifi_ok, 3, 5, 0, 16);
        sleep_ms(2000);
    } else {
        char *texto_wifi_erro[] = { "Erro WiFi", "Continuando", "modo offline..." };
        display_text(ssd, &frame_area, texto_wifi_erro, 3, 5, 0, 16);
        sleep_ms(2000);
    }

    // Inicializa o temporizador para envios TCP
    ultimo_envio_tcp = time_us_32() / 1000;

    // Loop principal: mede, exibe e verifica as condições de alerta
    while (true) {
        sleep_ms(1000);
        
        // Leitura e formatação da temperatura e dos alertas
        float temperatura = leitura_temp_precisa(200);
        char temp_str[20];
        sprintf(temp_str, "Temp: %.2f C", temperatura); //Exibe a temperatura
        char alert_str[20];
        sprintf(alert_str, "Alertas: %d", alert_count); //Exibe o número de alertas
        char limit_str[20];
        sprintf(limit_str, "Limite: %.1f C", temp_limite_config); //Exibe o limite configurado pela pessoa
        char wifi_str[20];
        sprintf(wifi_str, "WiFi: %s", wifi_conectado ? "ON" : "OFF"); //Exibe status do WiFi
        
        // Atualiza o display OLED com as leituras
        memset(ssd, 0, ssd1306_buffer_length);
        char *temp_text[] = { temp_str, alert_str, limit_str, wifi_str };
        display_text(ssd, &frame_area, temp_text, 4, 5, 0, 16);
        printf("Temperatura: %.2f C | Limite: %.1f C | Alerta: %s | Alertas: %d\n",
               temperatura, temp_limite_config,
               alerta_ativo ? "Ativo" : "Inativo",
               alert_count);

        // Ajuste do limite via botões: BT_A para aumentar; BT_B para diminuir.
        if (!gpio_get(BT_A)) {
            temp_limite_config += 1.0f;
            sleep_ms(200);
        }
        if (!gpio_get(BT_B)) {
            temp_limite_config -= 1.0f;
            sleep_ms(200);
        }

        // Controle do alerta com histerese:
        // Se a temperatura exceder o limite e o alerta não estiver ativo, ativa o alerta.
        // Se a temperatura cair abaixo do limite menos a histerese, desativa o alerta.
        if (temperatura >= temp_limite_config && !alerta_ativo) {
            alerta_ativo = true;
            alert_count++;
            // Acende todos os LEDs em vermelho
            for (int i = 0; i < LED_COUNT; i++) {
                npSetLED(i, 30, 0, 0);
            }
            npWrite();
            start_beep(BUZZER_PIN_A);
            start_beep(BUZZER_PIN_B);
            
            // Se WiFi conectado, envia imediatamente dados ao servidor
            if (wifi_conectado) {
                enviar_temperatura_tcp(temperatura, temp_limite_config, alerta_ativo, alert_count);
                ultimo_envio_tcp = time_us_32() / 1000;
            }
        } else if (temperatura < (temp_limite_config - DELTA_HISTERESE) && alerta_ativo) {
            alerta_ativo = false;
            // Apaga os LEDs
            for (int i = 0; i < LED_COUNT; i++) {
                npSetLED(i, 0, 0, 0);
            }
            npWrite();
            stop_beep(BUZZER_PIN_A);
            stop_beep(BUZZER_PIN_B);
            
            // Se WiFi conectado, envia imediatamente dados ao servidor
            if (wifi_conectado) {
                enviar_temperatura_tcp(temperatura, temp_limite_config, alerta_ativo, alert_count);
                ultimo_envio_tcp = time_us_32() / 1000;
            }
        }
        
        // Verificar se é hora de enviar dados via TCP (a cada TCP_INTERVAL_MS)
        uint32_t agora = time_us_32() / 1000;
        if (wifi_conectado && (agora - ultimo_envio_tcp >= TCP_INTERVAL_MS)) {
            enviar_temperatura_tcp(temperatura, temp_limite_config, alerta_ativo, alert_count);
            ultimo_envio_tcp = agora;
        }
    }

    // Este ponto nunca será alcançado, mas se fosse, desligaria o WiFi
    if (wifi_conectado) {
        wifi_deinit();
    }

    return 0;
}
