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
        npSetLED(2,0,0,35);
        npSetLED(6,0,0,35);
        npSetLED(8,0,0,35);
        npSetLED(10,0,0,35);
        npSetLED(12,0,0,35);
        npSetLED(14,0,0,35);
        npSetLED(16,0,0,35);
        npSetLED(18,0,0,35);
        npWrite();
        
        sleep_ms(500);
    }
}
