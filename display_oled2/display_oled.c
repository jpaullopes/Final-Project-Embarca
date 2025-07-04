#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main()
{
    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

// Parte do código para exibir a mensagem no display (opcional: mudar ssd1306_height para 32 em ssd1306_i2c.h)
// /**
    char *text[] = {
        "  Bem-vindos!   ",
        "  Embarcatech   "};

    int y = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
// */

// Parte do código para exibir a linha no display (algoritmo de Bresenham)
/**
    ssd1306_draw_line(ssd, 10, 10, 100, 50, true);
    render_on_display(ssd, &frame_area);
*/


    const uint8_t bitmap_128x64[] = { 
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0xaa,0x0a,0x00,0x00,0x10,
 0x11,0x11,0x11,0x11,0x11,0x00,0x00,0x4a,0x4a,0x4a,0x4a,0x4a,0x0a,0x00,0x00,
 0x20,0x21,0x21,0x21,0x21,0x01,0x00,0x00,0x4a,0x94,0x94,0x94,0x94,0x04,0x00,
 0x00,0x10,0x21,0x22,0x22,0x22,0x12,0x00,0x00,0x4a,0x4a,0x44,0x44,0x44,0x04,
 0x00,0x00,0x90,0x90,0x92,0x92,0x92,0x02,0x00,0x00,0x22,0x25,0x24,0x24,0x24,
 0x14,0x00,0x00,0x14,0x92,0x92,0x92,0x92,0x02,0x00,0x00,0x00,0x00,0x48,0x04,
 0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
 0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
 0x50,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x84,0x04,0x00,0x00,0x00,0x00,0x00,
 0x00,0x28,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x94,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x04,0x00,0x00,0x00,
 0x00,0x00,0x00,0x48,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,
 0x00,0x00,0x00,0x00,0x40,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x02,0x00,
 0x00,0x00,0x00,0x00,0x00,0xa4,0x04,0x00,0x00,0x00,0x00,0x20,0x92,0x48,0x92,
 0x24,0x09,0x00,0x00,0x4a,0x49,0x92,0x48,0x92,0x04,0x00,0x00,0x10,0x84,0x44,
 0x22,0x44,0x10,0x00,0x00,0x4a,0x51,0x12,0x89,0x12,0x05,0x00,0x00,0x10,0x8a,
 0x48,0x52,0xa4,0x08,0x00,0x00,0x4a,0x11,0x91,0x88,0x08,0x05,0x00,0x00,0x20,
 0xa2,0x24,0x22,0xa5,0x10,0x00,0x00,0x4a,0x49,0x92,0x94,0x10,0x0a,0x00,0x00,
 0x10,0x12,0x49,0x22,0x4a,0x01,0x00,0x00,0xa0,0x24,0x22,0x89,0x10,0x02,0x00,
 0x00,0x10,0x91,0x88,0x50,0x4a,0x01,0x00,0x00,0x40,0x24,0x25,0x8a,0x24,0x00,
 0x00,0x00,0x80,0x92,0x48,0x11,0x91,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x01,0x24,0x04,0x00,0x00,0x00,0x00,
 0x4a,0x02,0x88,0x02,0x00,0x00,0x00,0x00,0x90,0x00,0x24,0x09,0x00,0x00,0x00,
 0x00,0x24,0x02,0x48,0x04,0x00,0x00,0x00,0x00,0x88,0x04,0x22,0x01,0x00,0x00,
 0x00,0x00,0x52,0x02,0x88,0x14,0x00,0x00,0x00,0x00,0x84,0x00,0x24,0x01,0x00,
 0x00,0x00,0x00,0x50,0x02,0x48,0x0a,0x00,0x00,0x00,0x00,0x24,0x04,0x24,0x00,
 0x00,0x00,0x00,0x00,0x84,0x02,0x90,0x0a,0x00,0x00,0x00,0x00,0x28,0x01,0x44,
 0x02,0x10,0x00,0x00,0x00,0x44,0x00,0x28,0x09,0x00,0x00,0x00,0x01,0x90,0x00,
 0x40,0x04,0x20,0x00,0x80,0x00,0x48,0x00,0x14,0x01,0x40,0x00,0x00,0x00,0x00,
 0x00,0xa0,0x00,0x90,0x00,0x40,0x01,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x02,
 0x00,0x00,0x00,0x00,0x40,0x01,0x40,0x01,0x00,0x00,0x00,0x00,0x10,0x02,0x28,
 0x00,0x00,0x00,0x00,0x00,0xa0,0x00,0x40,0x05,0x00,0x00,0x00,0x00,0x08,0x05,
 0x10,0x02,0x00,0x00,0x00,0x00,0x50,0x02,0xa0,0x08,0x00,0x20,0x00,0x00,0x24,
 0x01,0x10,0x25,0x00,0x00,0x00,0x00,0x92,0x00,0x40,0x48,0x00,0xa8,0x00,0x00,
 0x09,0x00,0x80,0x92,0x02,0x45,0x00,0x40,0xa4,0x00,0x40,0x24,0x54,0x28,0x05,
 0x00,0x11,0x00,0x00,0x89,0x22,0x05,0x00,0x00,0x44,0x00,0x00,0x22,0x94,0x08,
 0x00,0x00,0x28,0x00,0x00,0x49,0x21,0x01,0x00,0x00,0x00,0x00,0x00,0x24,0x8a,
 0x04,0x00,0x00,0x14,0x00,0x00,0x88,0x24,0x00,0x00,0x00,0x08,0x00,0x00,0x24,
 0x91,0x00,0x00,0x00,0x04,0x00,0x00,0x90,0x24,0x01,0x00,0x00,0x00,0x00,0x00,
 0x20,0x49,0x00,0x00,0x00,0x04,0x00,0x00,0x10,0x22,0x80,0xaa,0xaa,0x02,0x00,
 0x00,0x40,0x89,0x00,0x00,0x40,0x00,0x00,0x00,0x80,0x24,0x80,0xaa,0x2a,0x01,
 0x00,0x00,0x40,0x48,0x00,0x92,0x44,0x00,0x00,0x00,0x00,0x25,0x80,0x24,0x09,
 0x00,0x00,0x00,0x00,0x91,0x00,0x11,0x52,0x00,0x00,0x00,0x00,0x44,0x00,0xa2,
 0x08,0x00,0x00,0x00,0x00,0x29,0x80,0x14,0x05,0x00,0x00,0x00,0x00,0x84,0x00,
 0x21,0x10,0x00,0x00,0x00,0x00,0x50,0x00,0x4a,0x05,0x00,0x00,0x00,0x00,0x08,
 0x80,0x10,0x01,0x00,0x00,0x00,0x00,0x50,0x11,0xa5,0x04,0x00,0x00,0x00,0x00,
 0x80,0xa4,0x08,0x02,0x00,0x00,0x00,0x00,0x50,0x12,0x52,0x01,0x00,0x00,0x00,
 0x00,0x00,0x49,0x09,0x00,0x00,0x00,0x00,0x00,0x40,0x84,0xa4,0x00,0x00,0x00,
 0x00,0x00,0x80,0x52,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x25,0x00,0x00,
 0x00,0x00,0x00,0x00,0xa5,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x11,0x00,
 0x00,0x00,0x00,0x00,0x00,0x52,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x84,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x10,
 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00 };

    ssd1306_t ssd_bm;
    ssd1306_init_bm(&ssd_bm, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&ssd_bm);

    ssd1306_draw_bitmap(&ssd_bm, bitmap_128x64);


    while(true) {
        sleep_ms(1000);
    }

    return 0;
}
