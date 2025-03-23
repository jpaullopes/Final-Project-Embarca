#include "oled_display.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

/**
 * @brief Exibe um array de strings no display OLED, atualizando a área de renderização.
 * 
 * @param ssd Buffer de vídeo do display
 * @param area Região do display onde o texto será desenhado
 * @param text Array contendo as mensagens a serem exibidas
 * @param text_count Quantidade de linhas de texto
 * @param start_x Coordenada X inicial para desenhar o texto
 * @param start_y Coordenada Y inicial para desenhar o texto
 * @param line_height Altura entre as linhas do texto
 */
void display_text(uint8_t *ssd, struct render_area *area, char *text[], int text_count, int start_x, int start_y, int line_height) {
    int y = start_y;
    
    // Ajusta cada linha para garantir alinhamento com números negativos
    char formatted_text[4][32];  // Buffer para texto formatado
    
    for (int i = 0; i < text_count; i++) {
        // Se a linha contém temperatura, aplica formatação especial
        if (strstr(text[i], "Temp:") == text[i]) {
            float temp;
            sscanf(text[i], "Temp: %f", &temp);
            snprintf(formatted_text[i], sizeof(formatted_text[i]), "Temp:%+6.2f C", temp);
            ssd1306_draw_string(ssd, start_x, y, formatted_text[i]);
        }
        // Se a linha contém limite, aplica formatação especial
        else if (strstr(text[i], "Limite:") == text[i]) {
            float limite;
            sscanf(text[i], "Limite: %f", &limite);
            snprintf(formatted_text[i], sizeof(formatted_text[i]), "Limite:%+6.1f C", limite);
            ssd1306_draw_string(ssd, start_x, y, formatted_text[i]);
        }
        // Outras linhas mantêm formato original
        else {
            ssd1306_draw_string(ssd, start_x, y, text[i]);
        }
        y += line_height;
    }
    
    render_on_display(ssd, area);
}

/**
 * @brief Inicializa o display OLED com I2C
 * 
 * @param sda_pin Pino SDA para I2C
 * @param scl_pin Pino SCL para I2C
 * @param frame_area Área de renderização do display
 * @return Buffer de vídeo do display
 */
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