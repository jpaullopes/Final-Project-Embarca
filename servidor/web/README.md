# Sistema de Monitoramento de Temperatura

Este projeto consiste em um sistema de monitoramento de temperatura em tempo real que utiliza um Raspberry Pi Pico W para coletar dados de temperatura e um servidor web para visualização e análise.

## Visão Geral

O sistema captura dados de temperatura de um sensor conectado à placa Raspberry Pi Pico W, processa essas informações e as exibe em tempo real em uma interface web interativa. O sistema também gera alertas quando a temperatura ultrapassa um limite definido.

## Componentes do Sistema

### Hardware

- **Raspberry Pi Pico W**: Microcontrolador responsável pela leitura do sensor de temperatura e envio dos dados via comunicação serial
- **Sensor de temperatura**: Coleta os dados de temperatura do ambiente
- **Computador host**: Executa o servidor web e a interface de visualização

### Software

- **Servidor Flask**: Aplicativo web em Python que recebe dados da porta serial e disponibiliza via WebSocket
- **Interface Web**: Frontend em HTML/CSS/JavaScript que exibe os dados em gráficos e métricas
- **Firmware do Pico W**: Código que roda no microcontrolador e controla a leitura do sensor e comunicação

## Funcionalidades

- Monitoramento de temperatura em tempo real
- Visualização de dados através de gráficos interativos
- Sistema de alertas para temperaturas acima do limite configurável
- Desativação automática do alerta apenas quando a temperatura cair 2°C abaixo do limite
- Rastreamento de estatísticas (temperatura mínima, máxima e média)
- Controle do tempo de sessão de monitoramento
- Interface responsiva e visual para análise rápida de dados

## Estrutura do Projeto

```
/projeto_monitoramento/
├── app.py                 # Aplicativo Flask principal (servidor)
├── requirements.txt       # Dependências Python
├── static/                # Arquivos estáticos
│   ├── script.js          # Lógica do frontend e gráficos
│   └── style.css          # Estilos da interface web
└── templates/             # Templates HTML
    └── index.html         # Interface principal do usuário
```

## Instalação e Configuração

### Requisitos

- Python 3.6 ou superior
- Raspberry Pi Pico W com firmware instalado
- Sensor de temperatura compatível conectado ao Pico W
- Bibliotecas Python listadas em `requirements.txt`

### Instalação

1. Clone o repositório ou extraia os arquivos para o diretório desejado
2. Instale as dependências Python:

```bash
pip install -r requirements.txt
```

3. Conecte o Raspberry Pi Pico W ao computador via USB
4. Carregue o firmware adequado no Pico W

### Execução

1. Inicie o servidor Flask:

```bash
python app.py
```

2. Acesse a interface web abrindo `http://localhost:5000` no seu navegador

## Detalhes de Comunicação

O Raspberry Pi Pico W envia dados no formato:

```
Temperatura: XX.XX C | Limite: YY.Y C | Alerta: Estado | Alertas: N
```

Onde:
- `XX.XX` é o valor atual da temperatura
- `YY.Y` é o limite configurado para alerta
- `Estado` é "Ativo" ou "Inativo"
- `N` é o contador de alertas disparados

## Comportamento do Sistema de Alarme

O sistema possui uma lógica específica para o controle de alarmes:

1. Quando a temperatura ultrapassa o limite configurado, o alarme é ativado
2. O alarme permanece ativo mesmo se a temperatura cair abaixo do limite
3. O alarme só é desativado quando a temperatura cai para pelo menos 2°C abaixo do limite
4. Essa histerese de 2°C evita a ativação/desativação rápida em situações de flutuação de temperatura

## Solução de Problemas

### Porta Serial Não Detectada

- Verifique se o Raspberry Pi Pico W está corretamente conectado ao computador
- Verifique se os drivers do dispositivo estão instalados
- Execute o utilitário `test_connection.py` para diagnosticar problemas de conexão

### Servidor Web Não Inicia

- Verifique se todas as dependências foram instaladas corretamente
- Certifique-se de que a porta 5000 não está sendo usada por outro aplicativo
- Verifique os logs para identificar erros específicos

### Dados Não Aparecem no Gráfico

- Verifique se o Pico W está enviando dados no formato esperado
- Verifique a conexão WebSocket no console do navegador
- Reinicie o servidor e atualize a página

## Personalização

- O limite de temperatura pode ser ajustado no firmware do Raspberry Pi Pico W
- A interface visual pode ser customizada através dos arquivos CSS
- O comportamento dos gráficos pode ser modificado em `script.js`

## Licença

Este projeto é disponibilizado sob a licença MIT.

---

Desenvolvido como parte de um projeto de monitoramento de temperatura utilizando microcontroladores.
