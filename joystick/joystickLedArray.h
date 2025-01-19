#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ledsArray.h"

#define MATRIX_SIZE 5  // Matriz 5x5
#define MAX_POSITION 4

#define VRX_PIN 26  // Eixo X (ADC0)
#define VRY_PIN 27  // Eixo Y (ADC1)
#define SW_PIN 22   // Botão do joystick

// Posição do LED ativo (inicia no centro da matriz)
int pos_x = 2, pos_y = 2;

/**
 * Converte coordenadas (x, y) para índice na matriz.
 */
int getLEDIndex(int x, int y) {
    return x * MATRIX_SIZE + y;
}

/**
 * Atualiza a posição do LED com base no joystick.
 */
void updateLEDPosition() {
    adc_select_input(0);
    uint16_t x_value = adc_read();
    
    adc_select_input(1);
    uint16_t y_value = adc_read();
    
    bool button_pressed = !gpio_get(SW_PIN);

    // Movimento no eixo X
    if (x_value < 1000 && pos_x > 0) {
        pos_x--;
    } else if (x_value > 3000 && pos_x < MAX_POSITION) {
        pos_x++;
    }

    // Movimento no eixo Y
    if (y_value < 1000 && pos_y > 0) {
        pos_y--;
    } else if (y_value > 3000 && pos_y < MAX_POSITION) {
        pos_y++;
    }

    // Atualiza a matriz de LEDs
    npClear();
    npSetLED(getLEDIndex(pos_x, pos_y), 0, 3, 0); // Verde
    npWrite();

    sleep_ms(150);
}

/**
 * Inicializa o joystick.
 */
void joystickInit() {
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
}
