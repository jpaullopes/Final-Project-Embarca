# Desafios no Desenvolvimento da Conexão WiFi com Raspberry Pi Pico W

Este documento detalha os principais obstáculos técnicos enfrentados durante o desenvolvimento do projeto de conexão Wi-Fi entre o Raspberry Pi Pico W e um servidor Python, e como cada um foi superado através de soluções específicas.

## 1. Configuração da Pilha TCP/IP (lwIP)

### Problema Detalhado
O principal desafio inicial foi a configuração adequada da pilha TCP/IP lwIP (lightweight IP) para o Raspberry Pi Pico W. Os erros de compilação eram principalmente de dois tipos:

1. **Conflito de definição de tipos**: O erro mostrava `redefinition of 'struct timeval'`, indicando que a estrutura estava sendo definida duas vezes - uma vez pelo lwIP e outra pelo sistema de cabeçalhos padrão.

2. **Incompatibilidade de modos**: Um erro crítico `"If you want to use Sequential API, you have to define NO_SYS=0 in your lwipopts.h"` mostrava que estávamos tentando usar APIs de socket em um ambiente configurado para não ter sistema operacional, o que é incompatível.

Estes erros revelaram uma desconexão fundamental entre o modelo de programação que estávamos tentando usar e a arquitetura suportada pelo hardware.

### Solução Detalhada
Após investigação profunda sobre a arquitetura do lwIP e como ele é implementado no Pico W, descobrimos que o SDK do Raspberry Pi Pico oferece duas abordagens principais para trabalhar com o lwIP:

1. **Modo baseado em polling (NO_SYS=1)**: 
   - Este modo é projetado para sistemas sem RTOS (sistema operacional em tempo real)
   - Não utiliza threads ou semáforos
   - Requer chamadas explícitas e frequentes a `cyw43_arch_poll()` para processar pacotes de rede
   - Usa um modelo baseado em callbacks para eventos de rede

2. **Modo baseado em sockets (NO_SYS=0)**:
   - Fornece uma API de socket semelhante à POSIX
   - Requer um ambiente com threading e primitivas de sincronização
   - Mais familiar para programadores de socket tradicionais, mas mais pesado

Depois de várias tentativas, ficou claro que o modo de polling era a escolha correta para nosso caso de uso simples. Modificamos o arquivo `lwipopts.h` com as seguintes configurações cruciais:

```c
#define NO_SYS                      1  // Modo sem sistema operacional, usando polling
#define LWIP_SOCKET                 0  // Desabilita a API de sockets que causa conflitos
#define LWIP_NETCONN                0  // Desabilita a API de conexão de rede que depende de threads
#define MEM_LIBC_MALLOC             0  // Não usa malloc() da libc para gerenciamento de memória
```

Esta configuração evitou os conflitos de definição de estruturas e nos direcionou para usar a API TCP de baixo nível do lwIP, que é mais adequada para o ambiente limitado em recursos do Pico W.

## 2. Localização do Arquivo lwipopts.h

### Problema Detalhado
Mesmo após criar corretamente o arquivo `lwipopts.h` com as configurações necessárias, enfrentamos um erro recorrente durante a compilação:

```
fatal error: lwipopts.h: No such file or directory
#include "lwipopts.h"
         ^~~~~~~~~~~~
compilation terminated.
```

Esta mensagem indicava que o compilador não conseguia localizar o arquivo `lwipopts.h` nos caminhos de inclusão. Este é um problema comum em projetos baseados em CMake, onde os caminhos de inclusão podem não ser configurados corretamente. A complexidade do SDK do Pico, que utiliza múltiplos diretórios e componentes, tornava difícil identificar exatamente onde o arquivo deveria ser colocado ou como torná-lo visível para o compilador.

### Solução Detalhada
Após múltiplas tentativas, a solução envolveu dois componentes críticos:

1. **Colocação do arquivo**: Garantimos que o arquivo `lwipopts.h` estivesse localizado no diretório raiz do projeto.

2. **Configuração do CMake**: Modificamos o arquivo CMakeLists.txt para explicitamente adicionar o diretório do projeto aos caminhos de inclusão do compilador:

```cmake
target_include_directories(wifiConect PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}  # Adiciona o diretório raiz do projeto aos caminhos de inclusão
)
```

Esta adição garantiu que o compilador procurasse o arquivo `lwipopts.h` no diretório do projeto primeiro, antes de buscar nos diretórios padrão do sistema ou do SDK. É uma solução simples, mas crucial, pois o sistema de compilação do Pico SDK não adiciona automaticamente o diretório raiz do projeto aos caminhos de inclusão para todos os componentes.

## 3. Escolha da API Correta (Sockets vs. TCP de Baixo Nível)

### Problema Detalhado
Inicialmente, tentamos implementar a comunicação usando a API de sockets tradicional, familiar aos desenvolvedores de software de rede em ambientes como Linux ou Windows:

```c
// Tentativa de usar a API de sockets tradicional
int sock = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in server_addr;
// ... configuração do endereço ...
connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
send(sock, data, len, 0);
recv(sock, buffer, size, 0);
close(sock);
```

Esta abordagem resultou em múltiplos erros de compilação devido à incompatibilidade com o ambiente do Pico W. Os problemas específicos incluíam:

1. Funções de socket não definidas ou não linkadas corretamente
2. Dependências não satisfeitas para APIs que requerem um ambiente com threading
3. Inconsistências entre o modelo de polling necessário para o cyw43 e o modelo bloqueante das APIs de socket

Ficou claro que era necessário usar uma abordagem diferente que fosse compatível com a arquitetura baseada em eventos do lwIP no modo polling.

### Solução Detalhada
Após estudar a documentação e exemplos do lwIP, implementamos uma solução completamente baseada na API TCP de baixo nível, que utiliza callbacks para tratar eventos de rede:

```c
// Estrutura para manter o estado da conexão
typedef struct {
    struct tcp_pcb *pcb;
    bool complete;
    bool connected;
    char buffer[256];
} TCP_CLIENT_STATE;

// Configuração de callbacks
tcp_arg(pcb, &state);                         // Define o argumento passado aos callbacks
tcp_err(pcb, tcp_error_cb);                   // Callback para erros
tcp_connect(pcb, &server_addr, PORT, tcp_connected_cb); // Inicia conexão assíncrona

// No loop principal
while (!state.complete) {
    cyw43_arch_poll();  // Processa eventos de rede
    sleep_ms(10);       // Pequena pausa para economizar CPU
}
```

Esta abordagem:
1. Alinha-se perfeitamente com o modelo baseado em eventos do lwIP
2. Evita bloqueios e é compatível com o modelo de polling necessário
3. Utiliza callbacks para responder a eventos como "conexão estabelecida", "dados recebidos", "erro ocorrido"
4. É mais eficiente em termos de memória e CPU para dispositivos restritos como o Pico W

Embora o código seja mais complexo e menos intuitivo que a API de sockets tradicional, ele funciona de forma mais harmoniosa com a arquitetura do sistema, o que resultou em uma solução estável e eficiente.

## 4. Arquitetura e Bibliotecas do Projeto

### Problema Detalhado
A estrutura de bibliotecas e componentes do Raspberry Pi Pico SDK é complexa, com múltiplas camadas e opções de configuração. Durante o desenvolvimento, encontramos dificuldades específicas:

1. **Dependências circulares**: Algumas combinações de bibliotecas geravam dependências circulares durante a compilação.

2. **Versões incompatíveis**: Diferentes componentes do lwIP exigem configurações específicas de threading que eram incompatíveis entre si.

3. **Documentação fragmentada**: A documentação do Pico SDK é extensa, mas fragmentada, o que dificultava encontrar a combinação exata de bibliotecas necessárias para nossa configuração.

Um erro comum que enfrentamos era:
```
undefined reference to `lwip_htons'
undefined reference to `lwip_connect'
```
Indicando que as funções estavam sendo declaradas, mas não corretamente linkadas na biblioteca final.

### Solução Detalhada
Após múltiplos testes e consultas à documentação, identificamos a combinação correta de componentes para nossa configuração:

```cmake
target_link_libraries(wifiConect
    pico_stdlib             # Biblioteca base do Pico
    pico_cyw43_arch_lwip_poll # Implementação lwIP específica para o chip CYW43439 no modo polling
)
```

A escolha crucial aqui foi `pico_cyw43_arch_lwip_poll`, que é específica para:

1. O chip de Wi-Fi CYW43439 usado no Pico W
2. Uma implementação do lwIP configurada para o modo polling
3. Todas as funções de baixo nível do TCP que estávamos utilizando

Esta escolha garantiu que todas as dependências necessárias fossem satisfeitas e que as versões corretas das funções fossem linkadas. Também simplificamos significativamente o CMakeLists.txt, removendo dependências desnecessárias que estavam causando conflitos.

## 5. Formato dos Dados para Comunicação Cross-Platform

### Problema Detalhado
Um desafio significativo foi estabelecer um formato de dados que funcionasse bem entre o Pico W (programado em C) e o servidor Python. As diferenças entre as linguagens e plataformas apresentaram várias complicações:

1. **Limitações de memória**: O Pico W tem recursos limitados, o que dificulta o uso de bibliotecas pesadas de serialização JSON em C.

2. **Conversão de tipos**: Garantir que os valores numéricos fossem interpretados corretamente em ambos os sistemas (como números de ponto flutuante).

3. **Eficiência de processamento**: O parsing de formatos complexos poderia sobrecarregar o Pico W.

4. **Compatibilidade de caracteres**: Diferenças nas representações de strings entre C e Python.

Tentamos inicialmente usar uma biblioteca JSON leve para C, mas isso adicionava complexidade desnecessária e aumentava o uso de memória.

### Solução Detalhada
Após experimentar várias abordagens, optamos por uma solução elegante que balanceava simplicidade e funcionalidade:

No lado do Pico W (C), geramos uma string formatada semelhante a JSON usando aspas simples:

```c
snprintf(data, sizeof(data), 
         "{'device': 'pico_w', 'temperature': %.2f, 'humidity': %.2f}", 
         temp, hum);
```

Esta abordagem:
1. Usa apenas funções padrão de C (`snprintf`)
2. É eficiente em termos de memória e CPU
3. Produz uma saída legível por humanos para depuração
4. Gera um formato que pode ser facilmente convertido em JSON válido

No lado do servidor Python, convertemos esta string em JSON válido com uma simples substituição de aspas:

```python
# Converter de aspas simples para aspas duplas e fazer o parsing
json_str = data_str.replace("'", '"')
json_data = json.loads(json_str)
print(f"Temperature: {json_data.get('temperature', 'N/A')}°C")
print(f"Humidity: {json_data.get('humidity', 'N/A')}%")
```

Esta solução eliminou a necessidade de bibliotecas JSON pesadas no lado do Pico W enquanto mantinha a conveniência do parsing JSON no lado do Python, resultando em um sistema de comunicação leve, eficiente e robusto.

## Conclusão

O desenvolvimento deste projeto demonstrou a importância de entender profundamente a arquitetura subjacente ao trabalhar com sistemas embarcados como o Raspberry Pi Pico W. Os desafios enfrentados não eram simples problemas de codificação, mas questões fundamentais de design de sistema e compatibilidade entre componentes.

A combinação de:
- Configuração adequada do lwIP
- Escolha da API correta para networking
- Configuração apropriada do sistema de build
- Formato de dados eficiente para comunicação cross-platform

Resultou em um sistema robusto e eficiente que pode servir como base para aplicações IoT mais complexas. A experiência também destacou a importância da abordagem iterativa na resolução de problemas de sistemas embarcados, onde muitas vezes é necessário experimentar diferentes configurações para encontrar a solução ideal.