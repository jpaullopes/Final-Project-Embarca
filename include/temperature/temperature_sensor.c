#include "temperature_sensor.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>

/**
 * @brief Inicializa o sensor de temperatura (ADC interno do RP2040)
 */
void init_temperature_sensor(void) {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Canal do sensor de temperatura interno
}


/**
 * @brief Faz N leituras do canal ADC para obter a média de tensão e converte para temperatura em °C.
 * 
 * @param nro_amostras Número de leituras a serem realizadas
 * @return A temperatura calculada em graus Celsius
 */
float leitura_temp_precisa(int nro_amostras) {
    int32_t soma = 0;  // Usamos int32_t em vez de uint32_t para permitir valores negativos
    
    for (int i = 0; i < nro_amostras; i++) {
        soma += adc_read();
        sleep_us(50);
    }
    
    float media = soma / (float)nro_amostras;
    float voltage = media * (3.3f / 4095.0f);
    
    // A fórmula do sensor interno do RP2040 já suporta temperaturas negativas
    float temperatura = 27.0f - ((voltage - 0.706f) / 0.001721f);
    
    // Validação para evitar valores absurdos (sensor interno normalmente opera entre -20°C e +85°C)
    if (temperatura < -40.0f || temperatura > 100.0f) {
        printf("[AVISO] Leitura de temperatura suspeita: %+.2f°C\n", temperatura);
    }
    
    return temperatura;
}