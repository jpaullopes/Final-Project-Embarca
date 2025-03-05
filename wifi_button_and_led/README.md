# Raspberry Pi Pico W - Servidor Web com Botões e LED

Este projeto implementa um servidor web no Raspberry Pi Pico W que permite o controle remoto de um LED e monitora o estado de dois botões.

## Características

- Servidor web para controle de um LED via navegador
- Monitoramento de dois botões físicos
- Interface web responsiva com atualizações em tempo real
- API JSON para integração com outros sistemas
- Timestamps baseados no tempo desde a inicialização do dispositivo
- Reconexão automática ao Wi-Fi
- Debouncing de botões para leituras estáveis

## Requisitos de Hardware

- Raspberry Pi Pico W
- LED conectado ao pino 12
- Botão 1 conectado ao pino 5 (com resistor pull-up interno)
- Botão 2 conectado ao pino 6 (com resistor pull-up interno)

## Configuração

1. Configure as credenciais Wi-Fi no código:

```c
#define WIFI_SSID "seu_wifi"
#define WIFI_PASS "sua_senha"
```

2. Compile o código usando o SDK do Pico
3. Carregue o firmware compilado para o Raspberry Pi Pico W

## Uso

1. Conecte o Pico W à energia
2. Após conectar ao Wi-Fi, o dispositivo exibirá seu endereço IP no monitor serial
3. Acesse o servidor web navegando para o IP do Pico W em qualquer navegador
4. Use os links para controlar o LED e visualizar o estado dos botões

## API JSON

Para integração com outros sistemas, o dispositivo oferece uma API JSON em `/api/status` retornando:

```json
{
  "led_state": true|false,
  "button1": {"message": "texto", "timestamp": "HH:MM:SS", "state": true|false},
  "button2": {"message": "texto", "timestamp": "HH:MM:SS", "state": true|false}
}
```

## Notas Técnicas

- Os timestamps são baseados no tempo decorrido desde a inicialização do dispositivo e serão reiniciados a cada reinício
- O formato dos timestamps é HH:MM:SS

## Resolução de Problemas

- Se o dispositivo não conectar ao Wi-Fi, ele tentará reconectar automaticamente a cada 15 segundos
- Para reiniciar completamente, desconecte e reconecte a energia
