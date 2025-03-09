# 📡 Raspberry Pi Pico W - Biblioteca de Conexão WiFi

<div align="center">

[![Status do Projeto](https://img.shields.io/badge/Status-Estável-brightgreen)]()
[![Licença](https://img.shields.io/badge/Licença-MIT-blue)]()
[![SDK](https://img.shields.io/badge/Pico_SDK-2.1.0-red)]()

</div>

Uma biblioteca que simplifica a conectividade WiFi e comunicação TCP para o Raspberry Pi Pico W, projetada para ser reutilizável em múltiplos projetos IoT.

## 📋 Conteúdo

- [Visão Geral](#-visão-geral)
- [Recursos](#-recursos)
- [Requisitos](#-requisitos)
- [Estrutura do Projeto](#-estrutura-do-projeto)
- [Dependências](#-dependências)
- [Instalação](#-instalação)
- [Uso da Biblioteca](#-uso-da-biblioteca)
  - [Inicialização Básica](#inicialização-básica)
  - [Envio de Dados](#envio-de-dados)
  - [Exemplos Completos](#exemplos-completos)
- [Configuração do Servidor](#-configuração-do-servidor)
- [Solução de Problemas](#-solução-de-problemas)
- [Personalização Avançada](#-personalização-avançada)
- [Contribuição](#-contribuição)

## 🌐 Visão Geral

A biblioteca `pico_wifi_lib` abstrai toda a complexidade da configuração de rede do Raspberry Pi Pico W, oferecendo uma API simples para se conectar a redes WiFi e realizar comunicação TCP. Esta biblioteca foi projetada com foco em simplicidade, eficiência de memória e facilidade de uso para projetos IoT.

## ✨ Recursos

- ✅ **Inicialização simplificada**: Configura o hardware WiFi com uma única função
- ✅ **Gerenciamento de conexão**: Conecta e monitora automaticamente o estado da conexão
- ✅ **Comunicação TCP robusta**: Implementa padrões de comunicação TCP confiáveis
- ✅ **Formatação de dados flexível**: Envia tanto dados padronizados quanto personalizados
- ✅ **Mensagens de diagnóstico**: Fornece feedback detalhado sobre estado da conexão
- ✅ **Gestão de erros**: Tratamento de situações comuns de falha de rede
- ✅ **Reutilizável**: Facilmente incorporada em diversos projetos

## 🔧 Requisitos

- **Hardware**:
  - Raspberry Pi Pico W
  - Cabo micro-USB para alimentação e programação

- **Software**:
  - Raspberry Pi Pico SDK (versão 2.1.0 ou superior)
  - CMake (3.13 ou superior)
  - Compilador GCC para ARM (arm-none-eabi-gcc)
  - Um computador para executar o servidor Python (opcional)

## 📁 Estrutura do Projeto

```
pico_wifi_project/
├── pico_wifi_lib.h       # Arquivo de cabeçalho da biblioteca
├── pico_wifi_lib.c       # Implementação da biblioteca
├── lwipopts.h            # Configurações do lwIP (obrigatório)
├── wifiConect.c          # Exemplo de aplicação
├── server.py             # Servidor TCP para testes
├── CMakeLists.txt        # Configuração de compilação
└── pico_sdk_import.cmake # Script para importar o SDK do Pico
```

## 📦 Dependências

Esta biblioteca depende dos seguintes componentes do SDK do Raspberry Pi Pico:

1. **cyw43_arch**: Interface para o chip WiFi CYW43439
2. **lwIP**: Implementação leve do protocolo TCP/IP
3. **pico_stdlib**: Funções padrão do Raspberry Pi Pico

## 🔌 Instalação

### Passo 1: Preparação do Ambiente

Certifique-se de ter o SDK do Pico e as ferramentas necessárias instaladas. Consulte a [documentação oficial](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) para mais detalhes.

### Passo 2: Configuração dos Arquivos

1. Adicione os arquivos da biblioteca ao seu projeto:
   - `pico_wifi_lib.h`
   - `pico_wifi_lib.c`

2. **Importante**: Crie um arquivo `lwipopts.h` no diretório raiz do projeto com as seguintes configurações mínimas:

```c
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// Configuração fundamental - OBRIGATÓRIO
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0
#define MEM_LIBC_MALLOC             0

// Outras configurações necessárias
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define TCP_MSS                     1460
#define LWIP_DHCP                   1
#define LWIP_CALLBACK_API           1

#endif /* LWIPOPTS_H */
```

### Passo 3: Configure o CMakeLists.txt

```cmake
# Adiciona a biblioteca WiFi
add_library(pico_wifi_lib STATIC
    pico_wifi_lib.c
)

# Define as dependências da biblioteca
target_link_libraries(pico_wifi_lib
    pico_stdlib
    pico_cyw43_arch_lwip_poll
)

# Inclui os diretórios necessários
target_include_directories(pico_wifi_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Vincula sua aplicação à biblioteca
target_link_libraries(seu_aplicativo
    pico_stdlib
    pico_wifi_lib
)

# IMPORTANTE: Certifique-se de que o lwipopts.h seja encontrado
target_include_directories(seu_aplicativo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
```

## 🚀 Uso da Biblioteca

### Inicialização Básica

```c
#include "pico_wifi_lib.h"

int main() {
    stdio_init_all();
    
    // Aguardar inicialização do console USB
    sleep_ms(2000);
    
    // Inicializar o módulo WiFi
    if (!wifi_init()) {
        printf("Erro: Falha ao inicializar o módulo WiFi\n");
        return 1;
    }
    
    // Conectar à rede WiFi
    if (!wifi_connect("nome_da_rede", "senha_wifi", 30000)) {
        printf("Erro: Falha ao conectar à rede WiFi\n");
        wifi_deinit();
        return 1;
    }
    
    // Obter e imprimir o endereço IP
    char ip_addr[16];
    wifi_get_ip(ip_addr, sizeof(ip_addr));
    printf("Endereço IP obtido: %s\n", ip_addr);
    
    // Loop principal
    while (true) {
        // Seu código aqui
        sleep_ms(1000);
    }
    
    return 0;
}
```

### Envio de Dados

#### 1. Dados de temperatura e umidade (pré-formatados)

```c
// Gera automaticamente dados formatados de temperatura e umidade
wifi_send_data("192.168.1.100", 12345, DATA_TYPE_TEMP_HUMIDITY, NULL, 0);
```

#### 2. Dados personalizados

```c
// Enviar dados personalizados no formato JSON
const char *custom_data = "{'device': 'pico_w', 'status': 'online', 'message': 'Hello!'}";
wifi_send_data("192.168.1.100", 12345, DATA_TYPE_CUSTOM, custom_data, strlen(custom_data));
```

### Exemplos Completos

#### Monitoramento de Reconexão

```c
while (true) {
    // Verificar estado da conexão a cada ciclo
    if (!wifi_is_connected()) {
        printf("WiFi desconectado. Tentando reconectar...\n");
        if (!wifi_connect(WIFI_SSID, WIFI_PASSWORD, 10000)) {
            printf("Erro ao reconectar. Aguardando 10 segundos...\n");
            sleep_ms(10000);
            continue;
        }
    }
    
    // Enviar dados a cada 5 segundos
    wifi_send_data(SERVER_IP, SERVER_PORT, DATA_TYPE_TEMP_HUMIDITY, NULL, 0);
    sleep_ms(5000);
}
```

## 🖥️ Configuração do Servidor

O projeto inclui um servidor Python simples para testar a comunicação:

1. Abra um terminal no diretório do projeto
2. Execute o servidor:

```bash
python server.py
```

3. O servidor mostrará os endereços IP disponíveis:

```
===== Raspberry Pi Pico W - TCP Server =====
Starting server on port 12345...

Endereços IP disponíveis:
- 192.168.1.100
- 10.0.0.5

O Pico W deve conectar-se a um desses endereços IP.

Server escutando em todas as interfaces na porta 12345
Aguardando conexões...
```

4. Use um dos endereços IP exibidos na configuração do Pico W.

## ❓ Solução de Problemas

### Problemas Comuns e Soluções

| Problema | Possível Causa | Solução |
|----------|----------------|---------|
| Falha na compilação | `lwipopts.h` não encontrado | Verifique se o arquivo está na raiz do projeto e incluído corretamente no CMakeLists.txt |
| Erro de conexão WiFi | Credenciais incorretas | Verifique o SSID e a senha |
| Erro TCP (ERR_CONN) | Servidor indisponível ou IP incorreto | Verifique se o servidor está rodando e se o IP está correto |
| Erro TCP (ERR_ABRT) | Firewall ou sub-redes diferentes | Verifique se o Pico W e o servidor estão na mesma sub-rede |
| Erro `redefinition of struct timeval` | Configuração incorreta do lwIP | Verifique as definições no arquivo `lwipopts.h` |

## 🔍 Personalização Avançada

### Modificação do Formato de Dados

Para personalizar o formato dos dados enviados, modifique a função `wifi_send_data` em `pico_wifi_lib.c`:

```c
if (data_type == DATA_TYPE_TEMP_HUMIDITY) {
    // Personalizar formato dos dados
    snprintf(send_buffer, sizeof(send_buffer), 
             "{'device': '%s', 'temperature': %.2f, 'humidity': %.2f, 'battery': %d}",
             "seu_dispositivo", temp, hum, battery_level);
    // ...
}
```

### Adição de Novos Tipos de Dados

1. Adicione um novo tipo na enumeração em `pico_wifi_lib.h`:

```c
typedef enum {
    DATA_TYPE_TEMP_HUMIDITY,
    DATA_TYPE_CUSTOM,
    DATA_TYPE_SENSOR_ARRAY  // Novo tipo
} wifi_data_type_t;
```

2. Implemente o processamento na função `wifi_send_data` em `pico_wifi_lib.c`.

## 🤝 Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para:

- Reportar bugs
- Sugerir novas funcionalidades
- Enviar pull requests

---

<div align="center">
  
🔌 **Desenvolvido com ❤️ para o Raspberry Pi Pico W** 🔌

</div>