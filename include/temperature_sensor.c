#include "temperature_sensor.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"

void init_temperature_sensor(void) {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Canal do sensor de temperatura interno
}

float leitura_temp_precisa(int nro_amostras) {
    uint32_t soma = 0;
    for (int i = 0; i < nro_amostras; i++) {
        soma += adc_read();
        sleep_us(50);
    }
    float media = soma / (float)nro_amostras;
    float voltage = media * (3.3f / 4095);
    return 27.0f - ((voltage - 0.706f) / 0.001721f);
}