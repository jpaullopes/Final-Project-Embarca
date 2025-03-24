#ifndef BUZZER_CONTROL_H
#define BUZZER_CONTROL_H

#include <stdint.h>

/**
 * Configura o PWM para um pino específico, definindo parâmetros de frequência e duty cycle.
 * @param gpio pino de saída
 * @param slice_num identificador do slice de PWM
 * @param channel canal do PWM
 * @param duty_cycle valor de duty cycle para o canal
 */
void configurar_pwm(uint32_t gpio, uint32_t slice_num, uint32_t channel, uint16_t duty_cycle);

/**
 * Inicializa o PWM de um buzzer, definindo a frequência de operação.
 * @param pin pino onde o buzzer está conectado
 * @param frequency frequência do buzzer em Hz
 */
void pwm_init_buzzer(uint32_t pin, uint32_t frequency);

/**
 * Ativa o buzzer, gerando um som contínuo.
 * @param pin pino onde o buzzer está conectado
 */
void start_beep(uint32_t pin);

/**
 * Desativa o buzzer, interrompendo o som.
 * @param pin pino onde o buzzer está conectado
 */
void stop_beep(uint32_t pin);

#endif // BUZZER_CONTROL_H