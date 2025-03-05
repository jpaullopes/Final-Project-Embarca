#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
// #include "ssd1306.h"    // OLED desativado
#include "hardware/i2c.h"
#include "hardware/adc.h"

const uint I2C_SDA = 14; // Define os pinos SDA e SCL
const uint I2C_SCL = 15;
const uint LED_PIN = 12;

int main(void)
{
    // Inicializa o LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    stdio_init_all();

    // Inicializa ADC para o sensor de temperatura
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    // Não seleciona canal externo para não conflitar com o sensor interno

    /* Desativa OLED por enquanto:
    // Inicialização do I2C e OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 200);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();
    
    // Prepara a área de renderização do OLED
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    */

    while (true)
    {
        // Leitura do sensor de temperatura
        uint16_t raw = adc_read();
        float voltage = raw * (3.3f / 4095);
        float temperature = 27 - ((voltage - 0.706f) / 0.001721f);

        // Controle do LED: acende se temperatura > 20°C
        if (temperature > 20.0f)
            gpio_put(LED_PIN, 1);
        else
            gpio_put(LED_PIN, 0);

        /* Desativa saída OLED:
        // Limpa buffer do display e exibe informações
        memset(ssd, 0, ssd1306_buffer_length);
        char temp_str[20];
        sprintf(temp_str, "Temp: %.1f C", temperature);
        ssd1306_draw_string(ssd, 5, 0, temp_str);
        ssd1306_draw_string(ssd, 5, 8, "Bem-vindos!");
        ssd1306_draw_string(ssd, 5, 16, "Embarcatech");
        render_on_display(ssd, &frame_area);
        */
        
        sleep_ms(1000);
    }

    return 0;
}
