/**
 * Embarcatech adaptado de: 
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"


int main() {
    // Inicializa as interfaces de entrada e saída padrão (UART)
    stdio_init_all();
    // Inicializa o conversor analógico-digital (ADC)
    adc_init();
    
    // Configura os pinos GPIO 26 e 27 como entradas de ADC (alta impedância, sem resistores pull-up)
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Inicia o loop infinito para leitura e exibição dos valores do joystick
    while (1) {
        // Seleciona a entrada ADC 0 (conectada ao eixo X do joystick)
        adc_select_input(1);
        // Lê o valor do ADC para o eixo X
        uint adc_x_raw = adc_read();
        
        // Seleciona a entrada ADC 1 (conectada ao eixo Y do joystick)
        adc_select_input(0);
        // Lê o valor do ADC para o eixo Y
        uint adc_y_raw = adc_read();

        // Configuração da barra de exibição dos valores de X e Y no terminal
    // Mostra a posição do joystick de uma forma como esta: 
        // X: [            o             ]  Y: [              o         ]
        const uint bar_width = 40; // Define a largura da barra
        const uint adc_max = (1 << 12) - 1; // Valor máximo do ADC (12 bits, então 4095)
        
        // Calcula a posição do marcador 'o' na barra de X e Y com base nos valores lidos
        uint bar_x_pos = adc_x_raw * bar_width / adc_max;
        uint bar_y_pos = adc_y_raw * bar_width / adc_max;
        
        // Exibe a barra do eixo X no terminal
        printf("\rX: [");
        for (uint i = 0; i < bar_width; ++i)
            putchar(i == bar_x_pos ? 'o' : ' ');
        printf("]  Y: [");
        
        // Exibe a barra do eixo Y no terminal
        for (uint i = 0; i < bar_width; ++i)
            putchar(i == bar_y_pos ? 'o' : ' ');
        printf("]");
        
        // Pausa o programa por 50 milissegundos antes de ler novamente
        sleep_ms(50);
    }
}