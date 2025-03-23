#include "oled_display.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

void display_text(uint8_t *ssd, struct render_area *area, char *text[], int text_count, int start_x, int start_y, int line_height) {
    int y = start_y;
    for (int i = 0; i < text_count; i++) {
        ssd1306_draw_string(ssd, start_x, y, text[i]);
        y += line_height;
    }
    render_on_display(ssd, area);
}

uint8_t* initialize_oled_display(int sda_pin, int scl_pin, struct render_area *frame_area) {
    // Inicializa I2C para comunicação com o display OLED
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // Inicializa e configura o display OLED
    ssd1306_init();
    
    // Configura área de renderização para tela inteira
    frame_area->start_column = 0;
    frame_area->end_column = ssd1306_width - 1;
    frame_area->start_page = 0;
    frame_area->end_page = ssd1306_n_pages - 1;
    
    calculate_render_area_buffer_length(frame_area);
    
    // Aloca e limpa o buffer do display
    uint8_t *ssd = malloc(ssd1306_buffer_length);
    if (ssd == NULL) {
        printf("Erro: Não foi possível alocar memória para o buffer do display\n");
        return NULL;
    }
    
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, frame_area);
    
    return ssd;
}