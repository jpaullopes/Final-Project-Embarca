# Projeto Matriz Controlada

Este projeto foi desenvolvido como um exercício para treinar o uso dos botões do joystick (BT\_C) e dos botões normais (BT\_A e BT\_B) em conjunto com uma matriz de LEDs NeoPixel.

## 📌 Descrição

O sistema permite o controle de uma matriz de 25 LEDs utilizando três botões físicos:

- **BT\_A**: Move a posição do LED para a direita.
- **BT\_B**: Move a posição do LED para a esquerda.
- **BT\_C**: Alterna o estado do LED atual na matriz:
  - Se o LED já estiver salvo no array, ele será removido.
  - Caso contrário, a posição do LED será armazenada.

## ✨ Funcionalidades

- Inicialização dos LEDs e botões.
- Controle preciso da posição do LED ativo.
- Salvamento e remoção dinâmica de posições na matriz de LEDs.
- Atualização visual instantânea dos LEDs com base nas interações do usuário.

## 🚀 Como Usar

1. Inicialize o projeto e configure os pinos dos botões e LEDs.
2. Pressione **BT\_A** para mover a posição do LED para a direita.
3. Pressione **BT\_B** para mover a posição do LED para a esquerda.
4. Pressione **BT\_C** para salvar ou remover a posição do LED no array.
5. Observe a matriz de LEDs atualizando conforme as interações.

## 🛠️ Requisitos

- Biblioteca **NeoPixel** para controle dos LEDs.
- Configuração adequada dos pinos dos botões e LEDs.
- SDK **BitDogLab**: [BitDogLab SDK](https://github.com/BitDogLab/BitDogLab)

## 📂 Estrutura do Projeto

O código-fonte principal está localizado no arquivo:

[matrizcontrolada.c](./matrizcontrolada.c)

## 👨‍💻 Autor

Desenvolvido por João Paulo Lopes.

