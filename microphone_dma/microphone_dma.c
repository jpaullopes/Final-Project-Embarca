#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "neoPixel.c"

// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 96.f
#define SAMPLES 200 // Número de amostras que serão feitas do ADC.
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f/5.f) // Intervalos de volume do microfone.

// Pino e número de LEDs da matriz de LEDs.
#define LED_PIN 7
#define LED_COUNT 25
// linguagem C
#define MATRIX_ROWS 5
#define MATRIX_COLS 5

#define abs(x) ((x < 0) ? (-x) : (x))

// Define DEBUG para gerar menos mensagens de depuração
#define DEBUG_INTERVAL 100 // Aumentado de 20 para 100 ciclos para reduzir logs

// Canal e configurações do DMA
uint dma_channel;
dma_channel_config dma_cfg;

// Buffer de amostras do ADC.
uint16_t adc_buffer[SAMPLES];

void sample_mic();
float mic_power();
uint8_t get_intensity(float v);

// Função que acende uma linha até o LED de índice fornecido
void acendendoLinha(uint ledIndex, uint colorR, uint colorG, uint colorB) {
    if (ledIndex >= LED_COUNT) return; // Validação de limite
    
    uint row = ledIndex / MATRIX_COLS; // Determina a linha do LED
    uint start, end;
    
    if (row % 2 == 0) { // Linhas pares (0, 2, 4) - da esquerda para a direita
        start = row * MATRIX_COLS; // Primeiro LED da linha
        end = ledIndex; // Até o LED especificado
    } else { // Linhas ímpares (1, 3) - da direita para a esquerda
        start = (row + 1) * MATRIX_COLS - 1; // Último LED da linha
        end = ledIndex; // Até o LED especificado
    }
    
    // Acende os LEDs conforme direção da linha
    if (row % 2 == 0) {
        for (uint i = start; i <= end; i++) {
            npSetLED(i, colorR, colorG, colorB);
        }
    } else {
        for (uint i = start; i >= end && i < LED_COUNT; i--) {
            npSetLED(i, colorR, colorG, colorB);
        }
    }
}

/**
 * Acende um número específico de LEDs em uma linha específica, respeitando
 * o padrão zigzag da matriz (linhas pares da esquerda para direita,
 * linhas ímpares da direita para esquerda).
 * 
 * @param row Número da linha (0-4)
 * @param numLEDs Número de LEDs a acender na linha (1-5)
 * @param colorR Valor do componente vermelho (0-255)
 * @param colorG Valor do componente verde (0-255)
 * @param colorB Valor do componente azul (0-255)
 */
void acendeLinhaEscada(uint row, uint numLEDs, uint8_t colorR, uint8_t colorG, uint8_t colorB) {
    if (row >= MATRIX_ROWS || numLEDs > MATRIX_COLS) return;
    
    uint start, i;
    
    // Limita numLEDs ao máximo de 5
    if (numLEDs > MATRIX_COLS) numLEDs = MATRIX_COLS;
    
    // Calcula o índice do primeiro LED da linha
    if (row % 2 == 0) { // Linhas pares - da esquerda para direita
        start = row * MATRIX_COLS;
        
        // Acende os LEDs da esquerda para a direita
        for (i = 0; i < numLEDs; i++) {
            npSetLED(start + i, colorR, colorG, colorB);
        }
    } else { // Linhas ímpares - da direita para esquerda
        start = (row + 1) * MATRIX_COLS - 1; // Último LED da linha
        
        // Acende os LEDs da direita para a esquerda
        for (i = 0; i < numLEDs; i++) {
            npSetLED(start - i, colorR, colorG, colorB);
        }
    }
}

/**
 * Acende uma barra vertical de LEDs em formato de escadinha.
 * Para cada intensidade:
 * 1 - Acende toda a linha inferior (5 LEDs)
 * 2 - Acende também a segunda linha com 4 LEDs
 * 3 - Acende também a terceira linha com 3 LEDs
 * 4 - Acende também a quarta linha com 2 LEDs
 * 5 - Acende também a quinta linha com 1 LED
 */
void lightVerticalBar(uint intensity) {
    // Limpa a matriz antes
    npClear();
    
    // Limita a intensidade ao máximo de 5 níveis
    if (intensity > 5) intensity = 5;
    
    // Padrão de escada: cada linha acima tem um LED a menos
    if (intensity >= 1) acendeLinhaEscada(0, 5, 0, 0, 80);       // Linha inferior: 5 LEDs azuis
    if (intensity >= 2) acendeLinhaEscada(1, 4, 0, 80, 80);      // Segunda linha: 4 LEDs ciano
    if (intensity >= 3) acendeLinhaEscada(2, 3, 60, 60, 0);      // Terceira linha: 3 LEDs amarelos
    if (intensity >= 4) acendeLinhaEscada(3, 2, 80, 40, 0);      // Quarta linha: 2 LEDs laranja
    if (intensity >= 5) acendeLinhaEscada(4, 1, 80, 0, 0);       // Linha superior: 1 LED vermelho
    
    // Atualiza os LEDs uma única vez
    npWrite();
}

int main() {
  stdio_init_all();

  // Delay para o usuário abrir o monitor serial...
  sleep_ms(5000);

  // Preparação da matriz de LEDs.
  printf("Preparando NeoPixel...\n");
  
  npInit(LED_PIN, LED_COUNT);

  // Preparação do ADC.
  printf("Preparando ADC...\n");

  adc_gpio_init(MIC_PIN);
  adc_init();
  adc_select_input(MIC_CHANNEL);

  adc_fifo_setup(
    true, // Habilitar FIFO
    true, // Habilitar request de dados do DMA
    1, // Threshold para ativar request DMA é 1 leitura do ADC
    false, // Não usar bit de erro
    false // Não fazer downscale das amostras para 8-bits, manter 12-bits.
  );

  adc_set_clkdiv(ADC_CLOCK_DIV);

  printf("ADC Configurado!\n\n");

  printf("Preparando DMA...\n");

  // Tomando posse de canal do DMA.
  dma_channel = dma_claim_unused_channel(true);

  // Configurações do DMA.
  dma_cfg = dma_channel_get_default_config(dma_channel);

  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16); // Tamanho da transferência é 16-bits
  channel_config_set_read_increment(&dma_cfg, false); // Desabilita incremento do ponteiro de leitura
  channel_config_set_write_increment(&dma_cfg, true); // Habilita incremento do ponteiro de escrita
  
  channel_config_set_dreq(&dma_cfg, DREQ_ADC); // Usamos a requisição de dados do ADC

  // Amostragem de teste.
  printf("Amostragem de teste...\n");
  sample_mic();

  printf("Configuracoes completas!\n");

  printf("\n----\nIniciando loop...\n----\n");
  while (true) {
    // Realiza uma amostragem do microfone.
    sample_mic();

    // Pega a potência média da amostragem do microfone.
    float avg = mic_power();
    avg = 2.f * abs(ADC_ADJUST(avg)); // Ajusta para intervalo de 0 a 3.3V.

    // Aplicar uma pequena estabilização ao valor para evitar flutuações rápidas
    static float last_avg = 0;
    avg = (avg * 0.7f) + (last_avg * 0.3f); // Média ponderada: 70% atual, 30% anterior
    last_avg = avg;

    uint intensity = get_intensity(avg);
    uint intensity_mapped = (intensity > 5 ? 5 : intensity);
    
    lightVerticalBar(intensity_mapped);

    // Formato simplificado e consistente
    printf("%d %.4f\r\n", intensity, avg);
    
    // Reduzir número de mensagens de debug
    static uint32_t debug_counter = 0;
    if (++debug_counter % DEBUG_INTERVAL == 0) {  // Intervalo maior
        printf("DEBUG: Ciclo %lu\r\n", debug_counter);
    }
    
    sleep_ms(50);
  }
}

/**
 * Realiza as leituras do ADC e armazena os valores no buffer.
 */
void sample_mic() {
  adc_fifo_drain(); // Limpa o FIFO do ADC.
  adc_run(false); // Desliga o ADC (se estiver ligado) para configurar o DMA.

  dma_channel_configure(dma_channel, &dma_cfg,
    adc_buffer, // Escreve no buffer.
    &(adc_hw->fifo), // Lê do ADC.
    SAMPLES, // Faz SAMPLES amostras.
    true // Liga o DMA.
  );

  // Liga o ADC e espera acabar a leitura.
  adc_run(true);
  dma_channel_wait_for_finish_blocking(dma_channel);
  
  // Acabou a leitura, desliga o ADC de novo.
  adc_run(false);
}

/**
 * Calcula a potência média das leituras do ADC. (Valor RMS)
 */
float mic_power() {
  float avg = 0.f;

  for (uint i = 0; i < SAMPLES; ++i)
    avg += adc_buffer[i] * adc_buffer[i];
  
  avg /= SAMPLES;
  return sqrt(avg);
}

/**
 * Calcula a intensidade do volume registrado no microfone.
 * Versão melhorada para maior precisão.
 */
uint8_t get_intensity(float v) {
  uint count = 0;
  
  // Ajuste para tornar mais sensível a sons baixos
  const float step_size = ADC_STEP/20;
  const float threshold = 0.1f; // Valor mínimo para considerar som
  
  if (v < threshold) return 0;  // Ignorar valores muito baixos (ruído)
  
  while ((v -= step_size) > 0.f && count < 255)
    ++count;
  
  return count;
}
