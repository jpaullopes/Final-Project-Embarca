# Sistema de Monitoramento e Alerta de Temperatura

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
[![Build Status](https://img.shields.io/badge/build-pico--sdk-green.svg)](https://github.com/raspberrypi/pico-sdk)
[![Web Service](https://img.shields.io/badge/web-flask-blue.svg)](./web)

Bem-vindo ao Sistema de Monitoramento e Alerta de Temperatura!  
Esta solução interativa integra a medição local de temperatura realizada pelo Raspberry Pi Pico W com uma interface web responsiva para visualização e análise remota em tempo real.

---

## 📚 Sumário

- [Introdução](#introdução)
- [Documentação do Projeto](#documentação-do-projeto)
- [Recursos e Funcionalidades](#recursos-e-funcionalidades)
- [Arquitetura do Projeto](#arquitetura-do-projeto)
- [Interface Web](#interface-web)
- [Hardware](#hardware)
- [Como Usar](#como-usar)
- [Situações Críticas](#situações-críticas)
- [Licença](#licença)

---

## 🔍 Introdução

Este projeto foi desenvolvido para monitorar continuamente a temperatura ambiente utilizando:
- **Firmware no Raspberry Pi Pico W:** Realiza leituras precisas com o sensor ADC interno, exibe dados em um display OLED, ativa alertas através de LEDs e buzzer, e envia os dados via WiFi para um servidor.
- **Interface Web Interativa:** Um servidor Flask recebe os dados do Pico via TCP e os disponibiliza por WebSocket, permitindo a exibição em tempo real através de gráficos dinâmicos e métricas detalhadas.

Técnicas de filtragem são empregadas para estabilizar as medições e o sistema conta com histerese para evitar oscilações nos alertas, garantindo precisão mesmo em ambientes ruidosos.

---

## 📋 Documentação do Projeto

Este repositório contém uma documentação abrangente que explica:

- **Motivação e Contexto:** Detalhes sobre por que o projeto foi criado, os problemas que ele resolve e os benefícios que traz para os usuários.
- **Fundamentos Teóricos:** Explicação dos princípios técnicos por trás do sistema, incluindo técnicas de filtragem de sinais, implementação de histerese e conceitos de comunicação via WebSockets.
- **Escolhas de Design:** Justificativas para as decisões tomadas durante o desenvolvimento, desde a seleção de componentes até a arquitetura de software.
- **Análise de Requisitos:** Documentação dos requisitos funcionais e não-funcionais que nortearam a implementação do sistema.
- **Relatório de Testes:** Resultados dos testes realizados para garantir a precisão e confiabilidade das medições.
- **Possibilidades de Expansão:** Sugestões para futuros aprimoramentos e adaptações do projeto para diferentes cenários.

A documentação completa pode ser encontrada na pasta `/docs` deste repositório, contendo diagramas, especificações e orientações detalhadas para desenvolvedores e usuários.

---

## 🚀 Recursos e Funcionalidades

### Dispositivo (Raspberry Pi Pico W)
- **Monitoramento Contínuo:** Leitura periódica das temperaturas com precisão aumentada (média de múltiplas amostras)
- **Exibição Local:** Dados são mostrados no display OLED SSD1306
- **Ajuste Dinâmico:** Usuário pode ajustar o limite de temperatura através de botões físicos
- **Alerta Visual e Sonoro:** LEDs WS2818B RGB e buzzer PWM ativados quando a temperatura excede o limite
- **Conectividade WiFi:** Envio de dados para o servidor via TCP
- **Histerese:** Sistema evita oscilações de alerta com margem de 2°C para desativação do alarme
- **Operação Offline:** Continua funcionando mesmo sem conexão WiFi

### Interface Web
- **Dashboard Responsivo:** Adapta-se a diferentes tamanhos de tela
- **Visualização em Tempo Real:** Gráficos atualizados via WebSocket
- **Modo Claro/Escuro:** Tema ajustável conforme preferência do usuário
- **Estatísticas Detalhadas:** Mínima, máxima, média e tendência de temperatura
- **Indicadores Visuais:** Alertas e notificações interativas
- **Medidor Dinâmico:** Indicador gauge para visualização rápida do estado do sistema
- **Informações da Sessão:** Tempo de monitoramento e estatísticas da conexão

---

## 🛠 Arquitetura do Projeto

O projeto é dividido em dois módulos principais:

### Firmware (Raspberry Pi Pico W)
- **alertaTemperatura.c:** Lógica principal do sistema
- **Módulos organizados:**
  - **temperature_sensor:** Leitura do sensor ADC interno com filtragem
  - **oled_display:** Interface com o display OLED
  - **buzzer_control:** Controle do alerta sonoro via PWM
  - **ledsArray:** Gerenciamento da matriz de LEDs via PIO
  - **tcp_alerts/tcp_client:** Comunicação de rede para envio de dados

### Interface Web
- **Servidor Flask (app.py):** Recebe dados via TCP e fornece comunicação via WebSocket
- **Frontend:** HTML, CSS e JavaScript responsivos com recursos modernos
  - **Chart.js:** Biblioteca para gráficos interativos
  - **Socket.io:** Comunicação bidirecional em tempo real

---

## 🌐 Interface Web

A interface web oferece:

- **Painel de Controle:** Visualização rápida de todos os parâmetros importantes
- **Gráfico em Tempo Real:** Exibe temperatura, limite configurado e zona segura
- **Medidor Gauge:** Visualização intuitiva da temperatura atual em relação ao limite
- **Estatísticas da Sessão:** Tempo de monitoramento, valores mínimos, máximos e médios
- **Gráfico de Estatísticas:** Visualização comparativa dos valores principais
- **Indicadores de Estado:** Status da conexão e do alerta
- **Notificações:** Alertas visuais para eventos importantes
- **Tema Adaptativo:** Modo claro/escuro para melhor experiência visual
- **Layout Responsivo:** Funciona em dispositivos móveis e desktops

---

## 🔌 Hardware

### Componentes Utilizados:
- **Raspberry Pi Pico W:** Microcontrolador principal com WiFi integrado
- **Display OLED SSD1306:** Exibição local dos dados via I2C (pinos GPIO 14 e 15)
- **LEDs WS2818B:** Matriz de LEDs RGB controlada via PIO
- **Buzzer:** Alerta sonoro controlado por PWM (pinos GPIO 21 e 10)
- **Botões:** Ajuste de limite de temperatura (pinos GPIO 5 e 6)
- **Sensor de Temperatura:** Interno do RP2040 (ADC)

### Conexões:
- **I2C (Display):** SDA=GPIO14, SCL=GPIO15
- **Botões:** GPIO5 (aumentar limite), GPIO6 (diminuir limite)
- **Buzzer:** GPIO21 e GPIO10
- **LEDs RGB:** Via biblioteca PIO dedicada

---

## 💡 Como Usar

### Hardware e Firmware:
1. **Configuração do Hardware:**  
   - Conecte os componentes conforme definido no firmware:
     - Display OLED: Pinos I2C (GPIO 14 e 15)
     - Botões: GPIO 5 (aumentar) e GPIO 6 (diminuir)
     - Buzzer: GPIO 21 e 10
     - LEDs WS2818B conforme biblioteca PIO

2. **Compilação e Upload:**  
   - Utilize o CMake conforme configurado no `CMakeLists.txt`
   - Configure o WiFi modificando as constantes `WIFI_SSID` e `WIFI_PASSWORD`
   - Defina o IP do servidor em `SERVER_IP` e a porta em `TCP_PORT`

3. **Operação Local:**  
   - O display OLED exibirá: temperatura atual, limite configurado, contador de alertas e status do WiFi
   - Use os botões para ajustar o limite de temperatura
   - Quando a temperatura ultrapassar o limite, os LEDs ficarão vermelhos e o buzzer soará
   - O alerta só cessará quando a temperatura cair 2°C abaixo do limite (histerese)

### Interface Web:
1. **Instalação:**  
   - Navegue para o diretório `web` e instale as dependências:
     ```bash
     pip install -r requirements.txt
     ```

2. **Execução:**  
   - Inicie o servidor Flask:
     ```bash
     python app.py
     ```
   - O servidor informará o endereço IP e a porta para configurar no Pico W

3. **Acesso:**  
   - Abra o navegador no endereço informado pelo servidor (geralmente http://localhost:8080)
   - A interface exibirá automaticamente os dados quando o Pico W começar a enviá-los

---

## ⚠️ Situações Críticas

- **Alerta de Temperatura:**  
  Quando a temperatura excede o limite, o sistema:
  - Ativa os LEDs em vermelho
  - Aciona o buzzer com tom de alerta
  - Incrementa o contador de alertas
  - Envia imediatamente uma notificação para a interface web
  - Exibe alerta visual na interface (overlay vermelho pulsante)

- **Perda de Conexão:**
  - O sistema continua operando localmente mesmo sem WiFi
  - Tenta reconectar automaticamente a cada 10 ciclos (aproximadamente 3 segundos)
  - A interface web indica claramente quando a conexão é perdida

- **Filtragem de Ruído:**
  - O sistema realiza 100 leituras do ADC para cada medição, obtendo uma média para reduzir ruídos
  - A histerese de 2°C evita que pequenas flutuações causem múltiplos alarmes

---

## 📜 Licença

Este projeto está licenciado sob os termos da [Licença MIT](./LICENSE).

