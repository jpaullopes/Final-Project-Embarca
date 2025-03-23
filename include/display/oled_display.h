#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

/**
 * @file oled_display.h
 * @brief Interface para controle do display OLED SSD1306
 * 
 * Este módulo fornece funções para inicialização e controle
 * do display OLED SSD1306 usando comunicação I2C. Inclui
 * funções para renderização de texto e manipulação do buffer
 * de vídeo.
 */

#include "oled/ssd1306.h"
#include <stdint.h>

/**
 * @brief Exibe múltiplas linhas de texto no display OLED
 * 
 * @param ssd Buffer de vídeo do display
 * @param area Área de renderização no display
 * @param text Array de strings a serem exibidas
 * @param text_count Número de linhas de texto
 * @param start_x Posição X inicial do texto
 * @param start_y Posição Y inicial do texto
 * @param line_height Altura em pixels entre as linhas
 * 
 * Esta função permite exibir várias linhas de texto no display OLED,
 * com controle preciso do posicionamento e espaçamento. O buffer é
 * atualizado automaticamente após desenhar todo o texto.
 */
void display_text(uint8_t *ssd, struct render_area *area, char *text[], 
                 int text_count, int start_x, int start_y, int line_height);

/**
 * @brief Inicializa o display OLED e configura a comunicação I2C
 * 
 * @param sda_pin Pino GPIO para o sinal SDA do I2C
 * @param scl_pin Pino GPIO para o sinal SCL do I2C
 * @param frame_area Ponteiro para a estrutura que define a área de renderização
 * @return Ponteiro para o buffer de vídeo alocado, ou NULL em caso de erro
 * 
 * Esta função:
 * 1. Configura os pinos I2C e inicializa a comunicação
 * 2. Inicializa o display OLED
 * 3. Configura a área de renderização para tela inteira
 * 4. Aloca e limpa o buffer de vídeo
 */
uint8_t* initialize_oled_display(int sda_pin, int scl_pin, struct render_area *frame_area);

#endif // OLED_DISPLAY_H