/* ========== INCLUDES ==========
   Bibliotecas padrão, específicas do Pico e módulos de hardware.
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "inc/ledsArray.h" // para utilização da matriz de LEDs

/* ========== DEFINES ==========
   Configuração de pinos, IP e demais constantes.
*/
#define I2C_SDA 14
#define I2C_SCL 15

/* ========== FUNÇÕES AUXILIARES ==========
   Funções de suporte para exibir texto, medir temperatura e configurar PWM.
*/

// Exibe uma lista de strings no display OLED
void display_text(uint8_t *ssd, struct render_area *area, char *text[], int text_count, int start_x, int start_y, int line_height) {
    int y = start_y;
    for (int i = 0; i < text_count; i++) {
        ssd1306_draw_string(ssd, start_x, y, text[i]);
        y += line_height;
    }
    render_on_display(ssd, area);
}

// Executa múltiplas leituras do ADC para calcular uma média e converter tensão em temperatura.
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

// Configura o PWM para um pino específico, definindo wrap e ciclo de trabalho.
void configurar_pwm(uint gpio, uint slice_num, uint channel, uint16_t duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    pwm_set_wrap(slice_num, 255);
    pwm_set_chan_level(slice_num, channel, duty_cycle);
    pwm_set_enabled(slice_num, true);
}

/* ========== VARIÁVEIS GLOBAIS ==========
   Variáveis para controle do alerta de temperatura.
*/
volatile float current_temperature = 0.0f;
volatile int alert_counter = 0;
volatile bool led_alert_state = false;
repeating_timer_t alert_timer; // Timer para interrupção de alerta

/* ========== FUNÇÃO DE INTERRUPÇÃO ==========
   Callback do timer: se a temperatura (>25ºC) for ultrapassada, incrementa
   o contador de alertas e faz a matriz de LEDs piscar em vermelho.
*/
bool temp_alert_callback(repeating_timer_t *rt) {
    if (current_temperature > 25.0f) {
        alert_counter++;                     // Incrementa o contador de alertas
        led_alert_state = !led_alert_state;    // Alterna o estado para efeito de piscar

        // Atualiza todos os LEDs:
        // Se led_alert_state for true, os LEDs são acesos em vermelho (30,0,0);
        // Caso contrário, são apagados (0,0,0).
        for (int i = 0; i < LED_COUNT; i++) {
            npSetLED(i, led_alert_state ? 30 : 0, 0, 0);
        }
        npWrite();
    }
    return true; // Mantém o timer ativo
}

/* ========== FUNÇÃO MAIN ==========
   - Inicializa a comunicação padrão, ADC, I2C, display OLED e a matriz de LEDs.
   - Configura o timer de alerta que verifica a temperatura a cada 500ms.
   - Em loop, mede a temperatura, atualiza o display e exibe as leituras e contagem de alertas.
*/
int main() {
    stdio_init_all();

    // Inicialização do ADC para sensor de temperatura
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Inicialização do I2C para comunicação com o display OLED
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização e configuração do display OLED
    ssd1306_init();
    struct render_area frame_area = {
        start_column : 0,
        end_column   : ssd1306_width - 1,
        start_page   : 0,
        end_page     : ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Inicializa a matriz de LEDs
    npInit(LED_PIN);

    // Configura o timer de alerta a cada 500ms.
    add_repeating_timer_ms(500, temp_alert_callback, NULL, &alert_timer);

    // Exibição de mensagem inicial
    char *text[] = { "  Bem-vindos!   ", "  Embarcatech   " };
    display_text(ssd, &frame_area, text, 2, 5, 0, 8);

    // Loop principal: mede, exibe e transmite a temperatura
    while (true) {
        sleep_ms(1000);
        float temperature = leitura_temp_precisa(200);
        current_temperature = temperature;  // Atualiza a variável para uso na ISR

        char temp_str[20];
        sprintf(temp_str, "Temp: %.2f C", temperature);
        memset(ssd, 0, ssd1306_buffer_length);
        char *temp_text[] = { temp_str };
        display_text(ssd, &frame_area, temp_text, 1, 5, 0, 8);

        printf("Temperatura: %.2f C | Alertas: %d\n", temperature, alert_counter);
    }

    return 0;
}