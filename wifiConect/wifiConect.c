/**
 * @file wifiConect.c
 * @brief Exemplo de uso da biblioteca pico_wifi_lib para conexão WiFi e comunicação TCP com Raspberry Pi Pico W
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico_wifi_lib.h"

// Configurações WiFi - SUBSTITUA com suas informações
#define WIFI_SSID "Tomada preguicosa"
#define WIFI_PASSWORD "cachorro123"
#define SERVER_IP "192.168.3.6"  // Substitua pelo IP do seu servidor
#define SERVER_PORT 12345        // Porta onde o servidor Python está escutando

// Função principal
int main() {
    stdio_init_all();
    
    // Aguardar para que o console USB possa se estabelecer
    sleep_ms(2000);
    printf("\n\n===== Raspberry Pi Pico W - Demo Biblioteca WiFi =====\n\n");

    // Verificar se as credenciais WiFi foram configuradas
    if (WIFI_SSID[0] == 'T') { // Modificar isso para um check real em seus projetos
        printf("AVISO: Configure suas credenciais de WiFi no código antes de usar.\n");
    }
    
    // Inicializar o módulo WiFi
    if (!wifi_init()) {
        printf("Erro: Falha ao inicializar o módulo WiFi\n");
        return 1;
    }
    
    // Conectar à rede WiFi
    if (!wifi_connect(WIFI_SSID, WIFI_PASSWORD, 30000)) {
        printf("Erro: Falha ao conectar à rede WiFi\n");
        wifi_deinit();
        return 1;
    }
    
    // Obter e imprimir o endereço IP
    char ip_addr[16];
    wifi_get_ip(ip_addr, sizeof(ip_addr));
    printf("Endereço IP obtido: %s\n\n", ip_addr);
    
    // Loop principal - enviar dados a cada 5 segundos
    while (true) {
        // Verificar se o WiFi ainda está conectado
        if (!wifi_is_connected()) {
            printf("WiFi desconectado. Tentando reconectar...\n");
            if (!wifi_connect(WIFI_SSID, WIFI_PASSWORD, 10000)) {
                printf("Erro ao reconectar. Aguardando 10 segundos...\n");
                sleep_ms(10000);
                continue;
            }
        }
        
        // Enviar dados de temperatura e umidade (gerados automaticamente pela biblioteca)
        printf("Enviando dados para %s:%d...\n", SERVER_IP, SERVER_PORT);
        if (!wifi_send_data(SERVER_IP, SERVER_PORT, DATA_TYPE_TEMP_HUMIDITY, NULL, 0)) {
            printf("Erro ao enviar dados. Verificando conexão...\n");
            // Continue tentando na próxima iteração
        }
        
        // Enviar dados personalizados de exemplo
        const char *custom_data = "{'device': 'pico_w', 'status': 'online', 'message': 'Hello from Pico W!'}";
        printf("\nEnviando dados personalizados...\n");
        wifi_send_data(SERVER_IP, SERVER_PORT, DATA_TYPE_CUSTOM, custom_data, strlen(custom_data));
        
        printf("\nAguardando 5 segundos antes da próxima transmissão...\n\n");
        sleep_ms(5000);
    }
    
    // Código nunca chega aqui no loop infinito acima, mas é uma boa prática incluir
    wifi_deinit();
    
    return 0;
}