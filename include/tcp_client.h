/**
 * @file tcp_client.h
 * @brief Biblioteca para comunicação TCP via WiFi no Raspberry Pi Pico W
 */

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"

// Tamanho do buffer de comunicação TCP
#define TCP_BUF_SIZE 512

// Porta padrão para comunicação TCP
#define TCP_PORT 5000

// Estrutura principal do cliente TCP
typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[TCP_BUF_SIZE];
    int buffer_len;
    int sent_len;
    bool complete;
    bool connected;
    char *user_message;  // Mensagem a ser enviada
} TCP_CLIENT_T;

/**
 * @brief Inicializa o WiFi e conecta a uma rede
 * 
 * @param ssid Nome da rede WiFi
 * @param password Senha da rede WiFi
 * @param timeout_ms Tempo limite para conexão em milissegundos
 * @return true se conectado com sucesso, false caso contrário
 */
bool wifi_init_and_connect(const char* ssid, const char* password, uint32_t timeout_ms);

/**
 * @brief Envia uma mensagem via TCP para um servidor
 * 
 * @param server_ip IP do servidor (formato string "192.168.1.100")
 * @param port Porta do servidor
 * @param message Mensagem a ser enviada
 * @return true se a mensagem foi enviada com sucesso, false caso contrário
 */
bool tcp_send_message(const char* server_ip, uint16_t port, const char* message);

/**
 * @brief Finaliza a conexão WiFi
 */
void wifi_deinit(void);

#endif // TCP_CLIENT_H