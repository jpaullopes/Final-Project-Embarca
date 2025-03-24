#ifndef LEDS_ARRAY_H
#define LEDS_ARRAY_H

/**
 * @file ledsArray.h
 * @brief Interface para controle de matriz de LEDs WS2818B
 * 
 * Este módulo fornece funções para controle de uma matriz de LEDs
 * endereçáveis WS2818B utilizando a máquina PIO do RP2040 para
 * gerar o protocolo de comunicação necessário.
 */

#include "ws2818b.pio.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <stdlib.h>
#include <stdbool.h>

// Número total de LEDs na matriz
#define LED_COUNT 25

// Pino de dados para a matriz de LEDs
#define LED_PIN 7

/**
 * @brief Estrutura que representa um LED RGB
 */
struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
extern npLED_t leds[LED_COUNT];

/**
 * @brief Inicializa o hardware PIO para controle dos LEDs
 * 
 * @param pin Pino GPIO usado para dados
 * 
 * Configura uma máquina de estado PIO com o programa necessário
 * para gerar o protocolo de comunicação dos LEDs WS2818B.
 */
void npInit(uint pin);

/**
 * @brief Define a cor de um LED específico
 * 
 * @param index Índice do LED na matriz (0 a LED_COUNT-1)
 * @param r Intensidade do vermelho (0-255)
 * @param g Intensidade do verde (0-255)
 * @param b Intensidade do azul (0-255)
 * 
 * Os valores não são enviados para os LEDs até que npWrite() seja chamada.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);

/**
 * @brief Limpa (apaga) todos os LEDs da matriz
 * 
 * Define todos os LEDs com cor RGB(0,0,0). 
 * Requer chamada a npWrite() para efetivar a mudança.
 */
void npClear(void);

/**
 * @brief Envia os dados de cor para a matriz de LEDs
 * 
 * Transmite o buffer atual de cores para os LEDs físicos
 * usando o protocolo WS2818B via PIO.
 */
void npWrite(void);

/**
 * @brief Liga todos os LEDs na cor amarela
 * 
 * Facilita testes e debug, definindo todos os LEDs
 * com a cor RGB(30,30,0). Requer npWrite() após.
 */
void ligarTodosOsLEDs(void);

/**
 * @brief Liga todos os LEDs com cores aleatórias
 * 
 * Útil para efeitos visuais e testes, gera cores
 * aleatórias para cada LED. Requer npWrite() após.
 */
void ligarTodosLedsCoresAleatorias(void);

/**
 * @brief Verifica e remove um valor de um array
 * 
 * @param vetor Array de valores
 * @param tamanho Tamanho do array
 * @param valor Valor a ser removido
 * @return true se o valor foi encontrado e removido
 * @return false se o valor não foi encontrado
 * 
 * Função auxiliar para gerenciar conjuntos de LEDs
 */
bool verificarEExcluir(int vetor[], int tamanho, int valor);

#endif // LEDS_ARRAY_H