/**
 * @file pico_wifi_lib.h
 * @brief Biblioteca para gerenciar conexão WiFi e comunicação TCP no Raspberry Pi Pico W
 * 
 * Esta biblioteca fornece funções para conectar o Pico W a uma rede WiFi
 * e realizar comunicação TCP com um servidor. Ela utiliza a API de baixo nível
 * do lwIP para comunicação em rede e é projetada para trabalhar no modo polling.
 * 
 * @author Seu Nome
 * @date Julho 2023
 */

#ifndef PICO_WIFI_LIB_H
#define PICO_WIFI_LIB_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Estrutura para armazenar dados do cliente TCP
 * 
 * Esta estrutura mantém o estado de uma conexão TCP, incluindo o
 * bloco de controle de protocolo, buffers e flags de estado.
 */
typedef struct {
    struct tcp_pcb *pcb;   /**< Bloco de controle de protocolo TCP */
    bool complete;         /**< Flag indicando se a transmissão foi concluída */
    bool connected;        /**< Flag indicando se está conectado ao servidor */
    char buffer[256];      /**< Buffer para armazenar dados recebidos */
    void *user_data;       /**< Dados adicionais definidos pelo usuário */
} wifi_tcp_client_t;

/**
 * @brief Tipos de dados suportados para envio
 */
typedef enum {
    DATA_TYPE_TEMP_HUMIDITY,  /**< Dados de temperatura e umidade */
    DATA_TYPE_CUSTOM          /**< Dados personalizados */
} wifi_data_type_t;

/**
 * @brief Inicializa o módulo WiFi
 * 
 * Inicializa o hardware WiFi do Pico W e prepara para conexões.
 * 
 * @return true se inicialização foi bem sucedida, false caso contrário
 */
bool wifi_init(void);

/**
 * @brief Conecta à rede WiFi
 * 
 * @param ssid Nome da rede WiFi
 * @param password Senha da rede WiFi
 * @param timeout_ms Tempo limite para conexão em milissegundos
 * @return true se conexão foi bem sucedida, false caso contrário
 */
bool wifi_connect(const char* ssid, const char* password, uint32_t timeout_ms);

/**
 * @brief Retorna o endereço IP atribuído ao Pico W
 * 
 * @param ip_buffer Buffer para armazenar o endereço IP como string
 * @param buffer_size Tamanho do buffer
 */
void wifi_get_ip(char* ip_buffer, size_t buffer_size);

/**
 * @brief Envia dados para um servidor TCP
 * 
 * @param server_ip Endereço IP do servidor
 * @param server_port Porta do servidor
 * @param data_type Tipo de dados a serem enviados
 * @param data Dados a serem enviados (para DATA_TYPE_CUSTOM)
 * @param data_len Tamanho dos dados (para DATA_TYPE_CUSTOM)
 * @return true se envio foi bem sucedido, false caso contrário
 */
bool wifi_send_data(const char* server_ip, uint16_t server_port, wifi_data_type_t data_type, 
                   const char* data, size_t data_len);

/**
 * @brief Desconecta e desliga o módulo WiFi
 */
void wifi_deinit(void);

/**
 * @brief Verifica se o WiFi está conectado à rede
 * 
 * @return true se conectado, false caso contrário
 */
bool wifi_is_connected(void);

#endif /* PICO_WIFI_LIB_H */