#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <stdbool.h>

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7
#define BT_A 05
#define BT_B 06
#define BT_C 22

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

void ligarTodosOsLEDs() {
    for (int i = 0; i < 25; i++) {
        npSetLED(i, 30, 30, 0); // Define a cor dos LEDs (30, 30, 0)
    }
}

void ligarTodosLedsCoresAleatorias() {
    for (int i = 0; i < 25; i++) {
        npSetLED(i, rand() % 10, rand() % 10, rand() % 10); 
    }
}

// Função para verificar se um LED já está no array e removê-lo se estiver
bool verificarEExcluir(int vetor[], int tamanho, int valor) {
    for (int i = 0; i < tamanho; i++) {
        if (vetor[i] == valor) {
            // Remove o valor do array deslocando os elementos
            for (int j = i; j < tamanho - 1; j++) {
                vetor[j] = vetor[j + 1];
            }
            vetor[tamanho - 1] = -1; // Define o último elemento como -1
            return true;
        }
    }
    return false;
}

int main() {

    //criando um vetor de 25 posições e inicializando com -1
    int vetor[25];
    for (int i = 0; i < 25; i++) {
        vetor[i] = -1;
    }

  // Inicializa entradas e saídas.
  stdio_init_all();

  // Inicializa matriz de LEDs NeoPixel.
  npInit(LED_PIN);
  npClear();

  // Inicializando botão
  gpio_set_dir(BT_A, GPIO_IN);
  gpio_pull_up(BT_A);
  gpio_set_dir(BT_B, GPIO_IN);
  gpio_pull_up(BT_B);
  gpio_set_dir(BT_C, GPIO_IN);
  gpio_pull_up(BT_C);

  // Variáveis de posição
  int posicaoA = 0;

  // Loop principal
  while (true) {
    //liga os leds salvos no vetor
      if (gpio_get(BT_A) == 0) {
          // Código para mover para a direita
          if (posicaoA < 25) {
              posicaoA++;
              npClear();
              npSetLED(posicaoA, 30, 30, 0);
              npWrite(); // Escreve os dados nos LEDs
          }
          sleep_ms(200);
      }
      if (gpio_get(BT_B) == 0) {
          // Código para mover para a esquerda
          if (posicaoA > 0) {
              posicaoA--;
              npClear();
              npSetLED(posicaoA, 30, 30, 0);
              npWrite(); // Escreve os dados nos LEDs
          }
          sleep_ms(200);
      }
      if(gpio_get(BT_C) == 0) {
        // Verifica se o LED atual já está no array e o remove se estiver
        if (!verificarEExcluir(vetor, 25, posicaoA)) {
            // Salva a posição do LED se não estiver no array
            vetor[posicaoA] = posicaoA;
        }
        npClear();
        for (int i = 0; i < 25; i++) {
            if (vetor[i] != -1) {
                npSetLED(vetor[i], 30, 30, 0); // Define a cor dos LEDs (30, 30, 0)
            }
        }
        npWrite(); // Escreve os dados nos LEDs
        sleep_ms(200);
      }
  }
}