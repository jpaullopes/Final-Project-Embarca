#include "tcp_alerts.h"
#include "tcp_client.h"
#include <stdio.h>

/**
 * @brief Envia dados de temperatura via TCP para o servidor.
 * 
 * @param temperatura A temperatura atual medida em °C
 * @param limite O limite configurado para alertas
 * @param alerta Status do alerta (true = ativo, false = inativo)
 * @param num_alertas Quantidade de alertas ocorridos
 * @param server_ip Endereço IP do servidor (formato string)
 * @param port Porta TCP do servidor
 * @return true se o envio foi bem-sucedido, false em caso de falha
 */
bool enviar_temperatura_tcp(float temperatura, float limite, bool alerta, int num_alertas, const char* server_ip, int port) {
    // Formata os dados no formato esperado pelo servidor, incluindo o sinal
    char mensagem[256];
    snprintf(mensagem, sizeof(mensagem), 
             "Temperatura: %+.2f C | Limite: %+.1f C | Alerta: %s | Alertas: %d",
             temperatura, limite,
             alerta ? "Ativo" : "Inativo",
             num_alertas);
    
    // Envia a mensagem via TCP
    bool resultado = tcp_send_message(server_ip, port, mensagem);
    
    if (resultado) {
        printf("[TCP] Mensagem enviada com sucesso!\n");
    } else {
        printf("[TCP] Falha ao enviar mensagem\n");
    }
    
    return resultado;
}