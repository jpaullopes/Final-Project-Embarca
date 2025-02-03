#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "matrizLeds.h"

int main() {
    // Inicializa o sistema.
    stdio_init_all();
    npInit(LED_PIN);
    npClear(); // Garante que todos os LEDs comecem apagados.


    // Passo 2: Envia os valores do buffer para os LEDs físicos.
    npWrite(); // Agora os LEDs acendem com as cores especificadas.

    // Loop infinito (nenhuma atualização adicional).

    while (true) {
        npClear();
        sleep_ms(500);
        npSetLED(22, 30, 30, 0);
        npSetLED(17, 30, 30, 0);
        npSetLED(14, 30, 30, 0);
        npSetLED(12, 30, 30, 0);
        npSetLED(11, 30, 30, 0);
        npSetLED(10, 30, 30, 0);
        npSetLED(7, 30, 30, 0);
        npSetLED(2, 30, 30, 0);
        npSetLED(6, 30, 30, 0);
        npSetLED(16, 30, 30, 0);

        npWrite();
        
        sleep_ms(500);
    }
}
