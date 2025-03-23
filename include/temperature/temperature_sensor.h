#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

/**
 * @file temperature_sensor.h
 * @brief Interface para o sensor de temperatura interno do RP2040
 * 
 * Este módulo fornece funções para inicialização e leitura do sensor
 * de temperatura interno do microcontrolador RP2040, usando o ADC
 * para fazer as medições.
 */

/**
 * @brief Inicializa o sensor de temperatura interno do RP2040
 * 
 * Configura o ADC e habilita o sensor de temperatura interno.
 */
void init_temperature_sensor(void);

/**
 * @brief Realiza múltiplas leituras do sensor para maior precisão
 * 
 * @param nro_amostras Quantidade de amostras para calcular a média
 * @return float Temperatura em graus Celsius
 * 
 * A função faz N leituras do ADC e calcula a média para reduzir
 * o ruído nas medições. Depois converte a tensão média em temperatura
 * usando a fórmula de calibração do sensor interno.
 */
float leitura_temp_precisa(int nro_amostras);

#endif // TEMPERATURE_SENSOR_H