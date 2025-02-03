#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <stdbool.h>

#include "ledsArray.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.

// Definição do número de LEDs e pino.

#define BT_A 05
#define BT_B 06
#define BT_C 22

// Definição de pixel GRB

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