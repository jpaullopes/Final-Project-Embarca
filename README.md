# Sistema de Monitoramento e Alerta de Temperatura

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
[![Build Status](https://img.shields.io/badge/build-pico--sdk-green.svg)](https://github.com/raspberrypi/pico-sdk)
[![Web Service](https://img.shields.io/badge/web-flask-blue.svg)](./web)

Bem-vindo ao Sistema de Monitoramento e Alerta de Temperatura!  
Esta solução interativa integra a medição local de temperatura realizada pelo Raspberry Pi Pico com uma interface web para visualização e análise remota.

---

## 📚 Sumário

- [Introdução](#introdução)
- [Recursos e Funcionalidades](#recursos-e-funcionalidades)
- [Arquitetura do Projeto](#arquitetura-do-projeto)
- [Interface Web](#interface-web)
- [Como Usar](#como-usar)
- [Situações Críticas](#situações-críticas)
- [Futuras Melhorias](#futuras-melhorias)
- [Licença](#licença)

---

## 🔍 Introdução

Este projeto foi desenvolvido para monitorar continuamente a temperatura ambiente utilizando:
- **Firmware no Raspberry Pi Pico:** Realiza leituras do sensor ADC, exibe dados em um display OLED e ativa alertas através de LEDs e buzzer.
- **Interface Web Interativa:** Um servidor Flask recebe os dados do Pico via comunicação serial e os disponibiliza por WebSocket, permitindo a exibição em tempo real através de gráficos e métricas.

Tecnologias de filtragem são empregadas para estabilizar as medições, garantindo precisão mesmo em ambientes ruidosos.

---

## 🚀 Recursos e Funcionalidades

- **Monitoramento Contínuo:** Leitura periódica das temperaturas.
- **Exibição Local:** Dados são mostrados no display OLED.
- **Ajuste Dinâmico:** Usuário pode ajustar o limite de temperatura.
- **Interface Web:** Visualização remota com gráficos interativos, estatísticas (min/máx/média) e alertas via WebSocket.
- **Filtragem de Sinal:** Média das leituras do ADC para reduzir oscilações.

---

## 🛠 Arquitetura do Projeto

O projeto é dividido em dois módulos principais:

### Firmware (Raspberry Pi Pico)
- **alertaTemperatura.c:** Lógica principal para leitura do sensor, controle do display OLED, LEDs e buzzer.
- **ledsArray.h:** Gerenciamento da matriz de LEDs via PIO.
- **ssd1306.h / ssd1306_i2c.c / ssd1306_font.h:** Comunicação e renderização no display OLED.

### Interface Web
- **Servidor Flask (app.py):** Recebe dados da porta serial e fornece comunicação via WebSocket.
- **Frontend:** HTML, CSS e JavaScript (armazenados na pasta `web/static` e `web/templates`) que exibem gráficos e métricas em tempo real.

---

## 🌐 Interface Web

A interface web permite:
- Visualizar os dados de temperatura em tempo real.
- Visualizar gráficos, estatísticas e histórico de leituras.
- Receber alertas visuais instantâneos caso a temperatura ultrapasse o limite configurado.

A estrutura do módulo web :
```
/c:/EmbarcaT/avante/web/
├── app.py                 # Servidor Flask
├── requirements.txt       # Dependências Python
├── static/                # Arquivos estáticos (CSS, JavaScript)
│   ├── script.js
│   └── style.css
└── templates/             # Templates HTML
    └── index.html
```

---

## 💡 Como Usar

### Hardware e Firmware:
1. **Configuração do Hardware:**  
   - Conecte o sensor de temperatura, display OLED, LEDs e buzzer conforme definido no firmware.
2. **Compilação e Upload:**  
   - Compile e carregue o firmware no Raspberry Pi Pico utilizando o SDK do Pico.
3. **Operação Local:**  
   - O display OLED exibirá os dados atuais, alertas e limites configurados.

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
3. **Acesso:**  
   - Abra `http://localhost:5000` no navegador para visualizar a interface web interativa.

---

## ⚠️ Situações Críticas

- **Alerta de Sobretemperatura:**  
  Quando a temperatura excede o limite, os LEDs são ativados em vermelho, o buzzer emite som e a interface web exibe um alerta visual imediato.

- **Filtragem de Medidas:**  
  O sistema utiliza a média de múltiplas leituras do ADC, garantindo que pequenos ruídos não disparem alarmes falsos.

---

## 📜 Licença

Este projeto está licenciado sob os termos da [Licença MIT](./LICENSE).

---

