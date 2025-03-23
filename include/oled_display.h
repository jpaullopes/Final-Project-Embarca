#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>
#include "oled/ssd1306.h"

/**
 * Exibe um array de strings no display OLED, atualizando a área de renderização.
 * @param ssd buffer de vídeo do display
 * @param area região do display onde o texto será desenhado
 * @param text array contendo as mensagens a serem exibidas
 * @param text_count quantidade de linhas de texto
 * @param start_x coordenada X inicial para desenhar o texto
 * @param start_y coordenada Y inicial para desenhar o texto
 * @param line_height altura entre as linhas do texto
 */
void display_text(uint8_t *ssd, struct render_area *area, char *text[], int text_count, int start_x, int start_y, int line_height);

/**
 * Inicializa o display OLED com I2C
 * @param sda_pin Pino SDA para I2C
 * @param scl_pin Pino SCL para I2C
 * @return Buffer de vídeo do display
 */
uint8_t* initialize_oled_display(int sda_pin, int scl_pin, struct render_area *frame_area);

#endif // OLED_DISPLAY_H