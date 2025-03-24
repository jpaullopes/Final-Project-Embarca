#include "buzzer_control.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"

/**
 * @brief Configura o PWM para um pino específico, definindo parâmetros de frequência e duty cycle.
 * 
 * @param gpio Pino de saída
 * @param slice_num Identificador do slice de PWM
 * @param channel Canal do PWM
 * @param duty_cycle Valor de duty cycle para o canal
 */
void configurar_pwm(uint32_t gpio, uint32_t slice_num, uint32_t channel, uint16_t duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    pwm_set_wrap(slice_num, 255);
    pwm_set_chan_level(slice_num, channel, duty_cycle);
    pwm_set_enabled(slice_num, true);
}

/**
 * @brief Inicializa o PWM de um buzzer, definindo a frequência de operação.
 * 
 * @param pin Pino onde o buzzer está conectado
 * @param frequency Frequência do buzzer em Hz
 */
void pwm_init_buzzer(uint32_t pin, uint32_t frequency) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint32_t slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (frequency * 4096));
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

/**
 * @brief Ativa o buzzer, gerando um som contínuo.
 * 
 * @param pin Pino onde o buzzer está conectado
 */
void start_beep(uint32_t pin) {
    pwm_set_gpio_level(pin, 2048); // Ativa o buzzer
}

/**
 * @brief Desativa o buzzer, interrompendo o som.
 * 
 * @param pin Pino onde o buzzer está conectado
 */
void stop_beep(uint32_t pin) {
    pwm_set_gpio_level(pin, 0); // Desativa o buzzer
}