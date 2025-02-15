# Sistema de Monitoramento e Alerta de Temperatura

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
[![Build Status](https://img.shields.io/badge/build-pico--sdk-green.svg)](https://github.com/raspberrypi/pico-sdk)

Bem-vindo ao Sistema de Monitoramento e Alerta de Temperatura!  
Esta solução interativa permite a medição contínua de temperatura através de um sensor ADC, com exibição em um display OLED e alertas visuais/sonoros em situações críticas.

---

## 📚 Sumário

- [Introdução](#introdução)
- [Recursos e Funcionalidades](#recursos-e-funcionalidades)
- [Arquitetura do Projeto](#arquitetura-do-projeto)
- [Como Usar](#como-usar)
- [Situações Críticas](#situações-críticas)
- [Futuras Melhorias](#futuras-melhorias)
- [Licença](#licença)

---

## 🔍 Introdução

O Sistema de Monitoramento e Alerta de Temperatura foi desenvolvido para:
- Monitorar continuamente a temperatura ambiente.
- Exibir dados em um display OLED.
- Permitir ajuste dinâmico do limite de temperatura via botões.
- Alertar o usuário com LEDs e buzzer em caso de situações críticas.

*Técnicas de filtragem são aplicadas para suavizar as medições do ADC, garantindo precisão mesmo em ambientes ruidosos.*

---

## 🚀 Recursos e Funcionalidades

- **Monitoramento Contínuo:** Atualização periódica das medições de temperatura.
- **Exibição Interativa:** Visualização clara e organizada com display OLED.
- **Ajuste Dinâmico:** Usuário pode modificar o limite de temperatura em tempo real.
- **Alertas Imediatos:** Ativação de LEDs e buzzer ao detectar sobretemperatura.
- **Filtragem de Sinais:** Média de múltiplas amostras do ADC para reduzir oscilações.

---

## 🛠 Arquitetura do Projeto

O projeto é dividido em módulos bem estruturados:

- **alertaTemperatura.c:** Lógica principal e controle do hardware.
- **ledsArray.h:** Gerenciamento da matriz de LEDs via máquina PIO.
- **ssd1306.h / ssd1306_i2c.c:** Comunicação com o display OLED.
- **ssd1306_font.h:** Dados para renderização dos caracteres.

> **Dica:** Cada módulo foi organizado para facilitar a manutenção e expansão do sistema.

---

## 💡 Como Usar

1. **Configuração do Hardware:**
   - Conecte os sensores, display OLED, LEDs e buzzer conforme definido no código.

2. **Compilação e Upload:**
   - Compile o projeto utilizando o SDK do Pico.
   - Faça o upload do firmware para o microcontrolador.

3. **Operação:**
   - Observe a temperatura, alertas e limite configurado no display OLED.
   - Ajuste o limite de temperatura usando os botões BT_A e BT_B.

---

## ⚠️ Situações Críticas

- **Alerta de Sobretemperatura:**  
  Quando a temperatura ultrapassa o limite ajustado, todos os LEDs acendem em vermelho e o buzzer emite um som contínuo para notificar o usuário.

- **Filtragem de Medidas:**  
  Para evitar alarmes falsos, o sistema utiliza a média de várias leituras do ADC, garantindo estabilidade nas medições.

---

## 🔮 Futuras Melhorias

- **Sensor Dedicado:**  
  Substituição do sensor ADC por um sensor de temperatura dedicado para maior precisão.

- **Integração Wi-Fi:**  
  Conectividade para logging remoto dos dados e análise histórica via bancos de dados.

---

## 📜 Licença

Este projeto é licenciado sob os termos da [Licença MIT](./LICENSE).

---

Obrigado por utilizar o Sistema de Monitoramento e Alerta de Temperatura!  
Contribuições e sugestões são muito bem-vindas.
