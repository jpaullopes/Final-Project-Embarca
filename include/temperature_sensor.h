#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

/**
 * Inicializa o sensor de temperatura (ADC interno do RP2040)
 */
void init_temperature_sensor(void);

/**
 * Faz N leituras do canal ADC para obter a média de tensão e converte para temperatura em °C.
 * @param nro_amostras número de leituras a serem realizadas
 * @return A temperatura calculada em graus Celsius
 */
float leitura_temp_precisa(int nro_amostras);

#endif // TEMPERATURE_SENSOR_H