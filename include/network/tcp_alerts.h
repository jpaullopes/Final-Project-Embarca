#ifndef TCP_ALERTS_H
#define TCP_ALERTS_H

/**
 * @file tcp_alerts.h
 * @brief Módulo de envio de alertas de temperatura via TCP
 * 
 * Este módulo fornece uma interface de alto nível para envio
 * de mensagens formatadas contendo informações sobre temperatura
 * e alertas para um servidor TCP.
 */

#include <stdbool.h>

/**
 * @brief Envia dados de monitoramento de temperatura via TCP
 * 
 * @param temperatura Temperatura atual medida em °C
 * @param limite Limite configurado para disparo de alertas em °C
 * @param alerta Estado atual do alerta (true = ativo)
 * @param num_alertas Contador de alertas disparados
 * @param server_ip Endereço IP do servidor de destino
 * @param port Porta TCP do servidor
 * @return true se o envio foi bem sucedido
 * @return false se houve falha no envio
 * 
 * Esta função formata uma mensagem contendo todas as informações
 * relevantes do monitoramento de temperatura e envia via TCP para
 * o servidor especificado. O formato da mensagem é:
 * "Temperatura: XX.XX C | Limite: XX.X C | Alerta: XXX | Alertas: XX"
 */
bool enviar_temperatura_tcp(float temperatura, float limite, bool alerta, 
                          int num_alertas, const char* server_ip, int port);

#endif // TCP_ALERTS_H