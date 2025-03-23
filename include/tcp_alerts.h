#ifndef TCP_ALERTS_H
#define TCP_ALERTS_H

#include <stdbool.h>

/**
 * Envia os dados de temperatura para o servidor via TCP
 * @param temperatura Valor atual da temperatura
 * @param limite Valor do limite configurado
 * @param alerta Status do alerta (ativo ou inativo)
 * @param num_alertas Contador de alertas disparados
 * @param server_ip Endereço IP do servidor
 * @param port Porta TCP do servidor
 * @return true se a mensagem foi enviada com sucesso, false caso contrário
 */
bool enviar_temperatura_tcp(float temperatura, float limite, bool alerta, int num_alertas, 
                           const char* server_ip, int port);

#endif // TCP_ALERTS_H