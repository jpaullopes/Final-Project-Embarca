#include <stdio.h>
#include "pico/stdlib.h" //biblioteca para uso de funções de entrada e saída padrão
#include "hardware/pio.h" //biblioteca para controle da máquina PIO
#include <stdlib.h>
#include "ledsArray.h" //biblioteca com funções de controle da matriz de LEDs
#include "joystickLedArray.h" //biblioteca com funções de controle do joystick

#define LED_PIN 7 // Pino de controle da matriz de LEDs

int main() {
    stdio_init_all();
    npInit(LED_PIN);
    joystickInit();

    while (1) {
        updateLEDPosition();
    }
}
