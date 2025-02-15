# Projeto Avante: Monitoramento e Alerta de Temperatura

Bem-vindo ao Projeto Avante! Este sistema foi desenvolvido para monitorar continuamente a temperatura ambiente usando um sensor ADC e exibir as informações em um display OLED. Além disso, o sistema alerta o usuário em situações críticas através de uma matriz de LEDs e um buzzer.

---

## Sumário

- [Introdução](#introdução)
- [Recursos e Funcionalidades](#recursos-e-funcionalidades)
- [Arquitetura do Projeto](#arquitetura-do-projeto)
- [Uso do Sistema](#uso-do-sistema)
- [Situations Críticas e Boas Práticas](#situações-críticas-e-boas-práticas)
- [Futuras Melhorias](#futuras-melhorias)
- [Licença](#licença)

---

## Introdução

O Projeto Avante é uma solução interativa para monitoramento de temperatura que permite ao usuário ajustar dinamicamente o limite de alerta. Através do uso de técnicas de filtragem, o sistema minimiza as oscilações inerentes ao sensor ADC, proporcionando leituras mais estáveis e precisas.

---

## Recursos e Funcionalidades

- **Monitoramento Contínuo:** Leitura periódica do sensor ADC para medir a temperatura.
- **Exibição Interativa:** Dados exibidos em um display OLED, permitindo uma visão clara das temperaturas, alertas e limites configurados.
- **Ajuste Dinâmico do Limite:** O usuário pode aumentar ou diminuir o limite de temperatura através de botões de hardware.
- **Alertas para Situações Críticas:** Em caso de sobreaquecimento, o sistema ativa uma matriz de LEDs e um buzzer para alertar visual e sonoramente.
- **Filtragem de Sinal:** Técnica de média das leituras para minimizar as oscilações e melhorar a precisão da leitura.

---

## Arquitetura do Projeto

O projeto está organizado nos seguintes arquivos principais:

- **display_oled.c:** Responsável pela lógica principal, leitura do sensor, processamento dos dados e controle do display OLED, LEDs e buzzer.
- **ledsArray.h:** Biblioteca que gerencia a matriz de LEDs utilizando a máquina PIO.
- **ssd1306.h & ssd1306_i2c.c:** Implementam a comunicação e comandos para o display OLED.
- **ssd1306_font.h:** Contém a fonte bitmap para a renderização de caracteres no display.

Cada módulo foi cuidadosamente estruturado para facilitar manutenções e futuras expansões.

---

## Uso do Sistema

1. **Configuração do Hardware:**  
   - Conecte o sensor ADC, display OLED, LEDs e buzzer aos pinos configurados no código.
   - Garanta que a alimentação e as conexões estejam corretas.

2. **Compilação e Upload:**
   - Compile o projeto utilizando o SDK do Pico/SDK.
   - Faça upload do firmware para o microcontrolador.

3. **Operação em Tempo Real:**
   - O display OLED mostrará a temperatura atual, o número de alertas e o limite configurado.
   - Utilize os botões BT_A e BT_B para ajustar o limite de temperatura dinamicamente.

---

## Situações Críticas e Boas Práticas

- **Resposta a Sobretemperaturas:**  
  Quando a temperatura excede o limite ajustado, o sistema ativa os LEDs em vermelho e emite um som contínuo para alertar o usuário. Esse mecanismo garante uma resposta imediata em situações críticas.

- **Filtragem do Sinal do ADC:**  
  Para reduzir a influência de ruídos e oscilações, o sistema realiza múltiplas leituras do ADC e calcula uma média. Essa abordagem melhora a precisão da medição e evita falsos alarmes.

---

## Futuras Melhorias

Para tornar o sistema ainda mais robusto e escalável, as seguintes melhorias estão sendo consideradas:

- **Implementação de um Sensor Dedicado:**  
  A substituição do sensor ADC atual por um sensor de temperatura dedicado pode oferecer leituras mais precisas e confiáveis.

- **Integração com Bancos de Dados via Wi-Fi:**  
  Integrar o sistema com conectividade Wi-Fi permitirá o armazenamento e análise dos dados em bancos de dados remotos, abrindo caminho para monitoramento remoto e análise histórica.

---

## Licença

Este projeto está licenciado conforme especificado no arquivo [LICENSE](./LICENSE).

---

Obrigado por utilizar o Projeto Avante! Contribuições e sugestões são bem-vindas para evoluir ainda mais essa solução.
