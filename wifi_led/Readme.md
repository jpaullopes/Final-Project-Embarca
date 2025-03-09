# WIFI

A seguir será desenvolvido uma aplicação simples de uso das configurações de servidor HTTP com o uso do Wifi da placa raspberry pico w, sendo utilizada para acender e apagar um LED na BitDogLab.

## Introdução:

A Raspberry Pi Pico W oferece suporte à conectividade Wi-Fi, permitindo a implementação de funcionalidades avançadas, como servidores HTTP, utilizando a linguagem C e o SDK oficial da Raspberry Pi. Com essa capacidade, é possível criar aplicações que interajam diretamente com dispositivos, como smartphones, via redes Wi-Fi. A seguir, apresentamos um exemplo prático que demonstra como conectar a Pico W a uma rede Wi-Fi e configurar um servidor HTTP básico para interação com um celular ou computador.


Neste fluxograma, vamos destacar e detalhar as seguintes etapas principais em um novo documento chamado de pseudocódigo. 

![Figura](images/fluxograma_wifi_led.png)  


## Pré-requisitos

1. Ambiente Configurado:
    * Instale o Raspberry Pi Pico SDK.
    * Baixe e configure o ***lwIP*** (protocolo usado para suporte à rede no Pico W).

            i. Iniciar um projeto C/C++ na extensão da raspberry pi pico;

            ii. Mudar o tipo da placa para pico w;

            iii. Ir em “Pico wireless options” e clicar em background lwIP

    * Certifique-se de que as bibliotecas ***pico_cyw43_arch_lwip_threadsafe_background*** estão configuradas corretamente.

2. Credenciais de Wifi: Substitua o nome e a senha do Wi-Fi no código.

## Código completo em C:

```c
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>

#define LED_PIN 12          // Define o pino do LED
#define WIFI_SSID "NomeDaRedeWiFi"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "SenhaDaRedeWiFi" // Substitua pela senha da sua rede Wi-Fi

// Buffer para respostas HTTP
#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" \
                      "<!DOCTYPE html><html><body>" \
                      "<h1>Controle do LED</h1>" \
                      "<p><a href=\"/led/on\">Ligar LED</a></p>" \
                      "<p><a href=\"/led/off\">Desligar LED</a></p>" \
                      "</body></html>\r\n"



// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a requisição HTTP
    char *request = (char *)p->payload;

    if (strstr(request, "GET /led/on")) {
        gpio_put(LED_PIN, 1);  // Liga o LED
    } else if (strstr(request, "GET /led/off")) {
        gpio_put(LED_PIN, 0);  // Desliga o LED
    }

    // Envia a resposta HTTP
    tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);

    // Libera o buffer recebido
    pbuf_free(p);

    return ERR_OK;
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor TCP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}

int main() {
    stdio_init_all();  // Inicializa a saída padrão
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    // Inicializa o Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    printf("Wi-Fi conectado!\n");
    printf("Para ligar ou desligar o LED acesse o Endereço IP seguido de /led/on ou /led/off\n");

    // Configura o LED como saída
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Inicia o servidor HTTP
    start_http_server();
    
    // Loop principal
    while (true) {
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);
    }

    cyw43_arch_deinit();  // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}
```

**OBS: Não se esqueça de colocar o nome e senha da sua rede wifi no campo designado no início do código!**

## Arquivo CMake
Além do arquivo em C (extensão .c), é necessário configurar um arquivo CMake para compilar e executar o programa na Raspberry Pi Pico. Esse arquivo, geralmente chamado CMakeLists.txt, define as configurações de build do projeto, como as bibliotecas que serão usadas, os arquivos de origem e as especificações do sistema.

Aqui está um exemplo básico do CMakeLists.txt para este projeto:

## Cmakelists.txt
```Ruby
# Definir o nome do projeto
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_w_wifi_example)

# Inicializar o SDK do Raspberry Pi Pico
pico_sdk_init()

# Adicionar o executável principal
add_executable(pico_w_wifi_example
    main.c  # Substitua pelo nome do arquivo principal do seu código
)

# Vincular bibliotecas necessárias
target_link_libraries(pico_w_wifi_example
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
)

# Incluir os headers necessários
target_include_directories(pico_w_wifi_example PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Habilitar o suporte a USB e UART
pico_enable_stdio_usb(pico_w_wifi_example 1)
pico_enable_stdio_uart(pico_w_wifi_example 0)

# Criar arquivo UF2 para upload
pico_add_extra_outputs(pico_w_wifi_example)
```

## Como Usar

1. Coloque este arquivo CMakeLists.txt no mesmo diretório do arquivo main.c com o código-fonte.
2. Certifique-se de que o ambiente do Pico SDK está configurado.
Compile o projeto:
```bash
mkdir build
cd build
cmake ..
make
```

3. Após a compilação, o arquivo .uf2 estará disponível no diretório build. Faça o upload para a Pico W.
4. Com o código compilado e feito o upload na placa, é possível controlar o LED azul RGB central da placa pesquisando pelo endereço IP em que a placa está conectada em um navegador.
5. Ao entrar no endereço IP vai abrir uma página com o formato mostrado a seguir. Com isso clique em Ligar LED ou Desligar LED para enviar para a placa a sua necessidade:

![Figura](images/print_tela_wifi_http.png) 

**Nota: Caso a tela não apareça para você, apenas entre na página do endereço IP no buscador e adicione /led/on ao endereço para ligar o LED ou /led/off para desligar.**




