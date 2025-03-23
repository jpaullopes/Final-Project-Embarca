#include "tcp_alerts.h"
#include "../include/tcp_client.h"
#include <stdio.h>

bool enviar_temperatura_tcp(float temperatura, float limite, bool alerta, int num_alertas, const char* server_ip, int port) {
    // Formata os dados no formato esperado pelo servidor
    char mensagem[256];
    snprintf(mensagem, sizeof(mensagem), 
             "Temperatura: %.2f C | Limite: %.1f C | Alerta: %s | Alertas: %d",
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