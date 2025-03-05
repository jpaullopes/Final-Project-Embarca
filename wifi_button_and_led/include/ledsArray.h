#ifndef LEDS_ARRAY_H
#define LEDS_ARRAY_H

#include "ws2818b.pio.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <stdlib.h>
#include <stdbool.h>

#define LED_PIN 7
#define LED_COUNT 25

struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
extern npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
extern PIO np_pio;
extern uint sm;

// Protótipos das funções
void npInit(uint pin);
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
void npClear();
void npWrite();
void ligarTodosOsLEDs();
void ligarTodosLedsCoresAleatorias();
bool verificarEExcluir(int vetor[], int tamanho, int valor);

#endif // LEDS_ARRAY_H