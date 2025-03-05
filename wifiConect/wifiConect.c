#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h> // Usando lwip/inet.h em vez de arpa/inet.h

#define WIFI_SSID "SEU_SSID"         // Substitua pelo seu SSID Wi-Fi
#define WIFI_PASSWORD "SUA_SENHA"     // Substitua pela sua senha Wi-Fi
#define SERVER_IP_ADDRESS "192.168.1.XXX" // Substitua pelo IP da sua máquina local
#define SERVER_PORT 8080

void connect_wifi() {
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar cyw43_arch\n");
        while(1);
    }
    cyw43_arch_enable_sta_mode();

    printf("Conectando-se à rede Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha na conexão Wi-Fi.\n");
        cyw43_arch_deinit();
        while(1);
    } else {
        printf("Conectado com sucesso!\n");
    }
}

int main() {
    stdio_init_all();

    if (WIFI_SSID[0] == '\0' || WIFI_PASSWORD[0] == '\0') {
        printf("Por favor, configure o SSID e a senha do Wi-Fi no código.\n");
        return 1;
    }

    connect_wifi();

    struct sockaddr_in server_addr;
    int sock = -1;

    while (true) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            printf("Erro ao criar socket\n");
            goto error;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_addr.sin_addr) <= 0) { // Verificação de erro para inet_pton
            printf("Erro ao converter endereço IP\n");
            goto error;
        }

        printf("Conectando ao servidor...\n");
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            printf("Erro ao conectar\n");
            goto error;
        }

        printf("Conectado! Enviando dados...\n");

        float temperature = 25.3 + (float)rand() / RAND_MAX * 2.0;
        char data_buffer[64];
        snprintf(data_buffer, sizeof(data_buffer), "Temperatura: %.2f C", temperature);

        if (send(sock, data_buffer, strlen(data_buffer), 0) < 0) {
            printf("Erro ao enviar dados\n");
            goto error;
        }
        printf("Dados enviados: %s\n", data_buffer);

    error:
        if (sock >= 0) {
            close(sock);
        }
        sleep_ms(5000);
    }

    cyw43_arch_deinit();
    return 0;
}