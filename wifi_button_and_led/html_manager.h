#ifndef HTML_MANAGER_H
#define HTML_MANAGER_H

#include <stdio.h>
#include <string.h>
#include "include/ledsArray.h"

// Tamanho máximo da resposta HTTP
#define MAX_HTTP_RESPONSE_SIZE 8192

// Definição da estrutura ButtonState que estava faltando
typedef struct ButtonState {
    char message[100];
    char timestamp[20];
    bool state;
} ButtonState;

// Buffer para resposta HTTP
extern char http_response[MAX_HTTP_RESPONSE_SIZE];

// Estados dos botões e LED
extern bool led_state;
extern ButtonState button1;
extern ButtonState button2;

// Estado da matriz de LEDs (armazenado localmente)
extern uint8_t led_matrix_state[LED_COUNT][3]; // [R, G, B] para cada LED

// Cria uma representação HTML da matriz de LEDs 5x5
void create_led_matrix_html(char* buffer, size_t buffer_size) {
    char matrix_html[4096] = "";
    strcat(matrix_html, "<div class=\"led-grid\">");
    
    for (int i = 0; i < LED_COUNT; i++) {
        char led_color[20];
        // Converte os valores RGB para formato hexadecimal
        snprintf(led_color, sizeof(led_color), "#%02x%02x%02x", 
                 led_matrix_state[i][0], 
                 led_matrix_state[i][1], 
                 led_matrix_state[i][2]);
                 
        // Cria o elemento do LED com seu índice como data-index e cor atual como background
        char led_html[256];
        snprintf(led_html, sizeof(led_html), 
                 "<div class=\"led\" data-index=\"%d\" style=\"background-color: %s\" "
                 "onclick=\"toggleLed(%d)\"></div>", 
                 i, led_color, i);
        strcat(matrix_html, led_html);
        
        // Adiciona quebra de linha a cada 5 LEDs para formar uma matriz 5x5
        if ((i + 1) % 5 == 0 && i < 24) {
            strcat(matrix_html, "<br>");
        }
    }
    
    strcat(matrix_html, "</div>");
    
    // Copia para o buffer de saída
    strncpy(buffer, matrix_html, buffer_size - 1);
    buffer[buffer_size - 1] = '\0'; // Garante terminação nula
}

// Função para criar a resposta HTTP HTML completa
void create_html_response() {
    // Buffer temporário para a matriz de LEDs HTML
    char led_matrix_buffer[4096] = "";
    create_led_matrix_html(led_matrix_buffer, sizeof(led_matrix_buffer));
    
    snprintf(http_response, MAX_HTTP_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
             "<!DOCTYPE html>"
             "<html>"
             "<head>"
             "  <meta charset=\"UTF-8\">"
             "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
             "  <title>Raspberry Pi Pico W - Controle</title>"
             "  <style>"
             "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; max-width: 800px; margin: 0 auto; }"
             "    h1 { color: #2c3e50; }"
             "    .card { background: #f8f9fa; border-radius: 10px; padding: 15px; margin-bottom: 20px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }"
             "    .button { display: inline-block; background: #3498db; color: white; padding: 10px 15px; text-decoration: none; border-radius: 5px; margin-right: 10px; }"
             "    .button.on { background: #2ecc71; }"
             "    .button.off { background: #e74c3c; }"
             "    .button.refresh { background: #95a5a6; }"
             "    .status { margin-top: 20px; }"
             "    .button-status { padding: 10px; border-left: 5px solid #3498db; background: #ecf0f1; margin-bottom: 10px; }"
             "    .timestamp { color: #7f8c8d; font-size: 0.8em; }"
             "    .led-grid { display: inline-block; padding: 10px; background: #222; border-radius: 8px; }"
             "    .led { display: inline-block; width: 30px; height: 30px; margin: 5px; border-radius: 50%%; background-color: #333; cursor: pointer; transition: background-color 0.3s; }"
             "    .led:hover { opacity: 0.8; }"
             "    .controls { margin-top: 15px; }"
             "    .color-picker { margin: 10px 0; }"
             "  </style>"
             "  <script>"
             "    let currentColor = [30, 30, 0]; // Default color [R,G,B]"
             ""
             "    function toggleLed(index) {"
             "      fetch(`/led/matrix/${index}/${currentColor[0]}/${currentColor[1]}/${currentColor[2]}`)"
             "        .then(response => response.json())"
             "        .then(data => {"
             "          // Atualiza a aparência do LED na interface"
             "          const led = document.querySelector(`.led[data-index=\"${index}\"]`);"
             "          led.style.backgroundColor = `rgb(${currentColor[0]}, ${currentColor[1]}, ${currentColor[2]})`;"
             "        })"
             "        .catch(err => console.error('Erro:', err));"
             "    }"
             ""
             "    function setColor(r, g, b) {"
             "      currentColor = [r, g, b];"
             "      document.getElementById('current-color').style.backgroundColor = `rgb(${r},${g},${b})`;"
             "    }"
             ""
             "    function allOn() {"
             "      fetch('/led/matrix/all/on')"
             "        .then(response => window.location.reload())"
             "        .catch(err => console.error('Erro:', err));"
             "    }"
             ""
             "    function allOff() {"
             "      fetch('/led/matrix/all/off')"
             "        .then(response => window.location.reload())"
             "        .catch(err => console.error('Erro:', err));"
             "    }"
             ""
             "    function randomColors() {"
             "      fetch('/led/matrix/random')"
             "        .then(response => window.location.reload())"
             "        .catch(err => console.error('Erro:', err));"
             "    }"
             ""
             "    function autoRefresh() {"
             "      setTimeout(function() {"
             "        fetch('/update')"
             "          .then(response => response.json())"
             "          .then(data => {"
             "            document.getElementById('button1-message').innerText = data.button1.message;"
             "            document.getElementById('button1-time').innerText = data.button1.timestamp;"
             "            document.getElementById('button2-message').innerText = data.button2.message;"
             "            document.getElementById('button2-time').innerText = data.button2.timestamp;"
             "          })"
             "          .catch(err => console.error('Erro:', err));"
             "        autoRefresh();"
             "      }, 2000); // Atualiza a cada 2 segundos"
             "    }"
             ""
             "    window.onload = autoRefresh;"
             "  </script>"
             "</head>"
             "<body>"
             "  <h1>Raspberry Pi Pico W - Controle</h1>"
             "  <div class=\"card\">"
             "    <h2>Matriz de LEDs</h2>"
             "    %s"
             "    <div class=\"controls\">"
             "      <div class=\"color-picker\">"
             "        <p>Cor atual: <span id=\"current-color\" style=\"display: inline-block; width: 20px; height: 20px; background-color: rgb(30,30,0); border-radius: 50%%;\"></span></p>"
             "        <button class=\"button\" onclick=\"setColor(30, 0, 0)\">Vermelho</button>"
             "        <button class=\"button\" onclick=\"setColor(0, 30, 0)\">Verde</button>"
             "        <button class=\"button\" onclick=\"setColor(0, 0, 30)\">Azul</button>"
             "        <button class=\"button\" onclick=\"setColor(30, 30, 0)\">Amarelo</button>"
             "        <button class=\"button\" onclick=\"setColor(0, 30, 30)\">Ciano</button>"
             "        <button class=\"button\" onclick=\"setColor(30, 0, 30)\">Magenta</button>"
             "      </div>"
             "      <button class=\"button on\" onclick=\"allOn()\">Todos Ligados</button>"
             "      <button class=\"button off\" onclick=\"allOff()\">Todos Desligados</button>"
             "      <button class=\"button\" onclick=\"randomColors()\">Cores Aleatórias</button>"
             "    </div>"
             "  </div>"
             "  <div class=\"card\">"
             "    <h2>Controle do LED</h2>"
             "    <a href=\"/led/on\" class=\"button on\">Ligar LED</a>"
             "    <a href=\"/led/off\" class=\"button off\">Desligar LED</a>"
             "    <p>Estado atual: LED %s</p>"
             "  </div>"
             "  <div class=\"card\">"
             "    <h2>Estado dos Botões</h2>"
             "    <div class=\"button-status\">"
             "      <p><strong>Botão 1:</strong> <span id=\"button1-message\">%s</span></p>"
             "      <p class=\"timestamp\">Última atualização: <span id=\"button1-time\">%s</span></p>"
             "    </div>"
             "    <div class=\"button-status\">"
             "      <p><strong>Botão 2:</strong> <span id=\"button2-message\">%s</span></p>"
             "      <p class=\"timestamp\">Última atualização: <span id=\"button2-time\">%s</span></p>"
             "    </div>"
             "    <a href=\"/\" class=\"button refresh\">Atualizar Página</a>"
             "  </div>"
             "  <div class=\"card\">"
             "    <h2>Informações do Sistema</h2>"
             "    <p>API JSON disponível em <a href=\"/api/status\">/api/status</a></p>"
             "  </div>"
             "</body>"
             "</html>\r\n",
             led_matrix_buffer,
             led_state ? "LIGADO" : "DESLIGADO",
             button1.message, button1.timestamp,
             button2.message, button2.timestamp);
}

// Função para criar resposta JSON para API
void create_json_response() {
    snprintf(http_response, MAX_HTTP_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n"
             "{"
             "\"led_state\": %s,"
             "\"button1\": {\"message\": \"%s\", \"timestamp\": \"%s\", \"state\": %s},"
             "\"button2\": {\"message\": \"%s\", \"timestamp\": \"%s\", \"state\": %s},"
             "\"matrix\": {"
             "\"led_count\": %d"
             "}"
             "}",
             led_state ? "true" : "false",
             button1.message, button1.timestamp, button1.state ? "true" : "false",
             button2.message, button2.timestamp, button2.state ? "true" : "false",
             LED_COUNT);
}

#endif /* HTML_MANAGER_H */
