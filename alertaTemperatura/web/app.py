import eventlet
eventlet.monkey_patch()

from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
import threading
import re
import time
import logging
import socket
import os
from datetime import datetime

# Configuração de logging para melhor depuração
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(levelname)s - %(message)s',
                    datefmt='%H:%M:%S')

# Desativar logs desnecessários
logging.getLogger('werkzeug').setLevel(logging.WARNING)
logging.getLogger('engineio').setLevel(logging.WARNING)
logging.getLogger('socketio').setLevel(logging.WARNING)

# Criação do aplicativo Flask
app = Flask(__name__)
socketio = SocketIO(app, async_mode='eventlet', cors_allowed_origins="*")

# Flag para controlar o nível de verbosidade dos logs
VERBOSE_LOGGING = True

# IMPORTANTE: Use a mesma porta para HTTP/websocket E TCP
# O Pico está enviando dados para a porta 5000
TCP_PORT = 5000
WEB_PORT = 5001  # Vamos usar a mesma porta para tudo

# Variáveis globais para acompanhar estatísticas e dados
latest_data = {
    'temperatura': 0.0,
    'limite': 45.0,
    'status_alerta': 'Inativo',
    'alertas': 0,
    'timestamp': datetime.now().isoformat()
}
connection_count = 0
message_count = 0

# Função para extrair dados da mensagem do Pico W
def extract_data_from_message(data):
    """Extrai temperatura, limite, status de alerta e contador de alertas da mensagem"""
    try:
        # Log da mensagem recebida para debug
        logging.debug(f"Processando mensagem: '{data}'")
        
        # Extração via expressões regulares para formato complexo
        temp_match = re.search(r'Temperatura:\s*([0-9]+(?:\.[0-9]+)?)', data)
        limite_match = re.search(r'Limite:\s*([0-9]+(?:\.[0-9]+)?)', data)
        alerta_match = re.search(r'Alerta:\s*(\w+)', data)
        alertas_match = re.search(r'Alertas:\s*([0-9]+)', data)
        
        result = {
            'timestamp': int(datetime.now().timestamp()),
            'hora_formatada': datetime.now().strftime("%H:%M:%S")
        }
        
        if temp_match:
            temp = float(temp_match.group(1))
            # Verificação de sanidade
            if -50 <= temp <= 150:
                result['temperatura'] = temp
            else:
                logging.warning(f"Temperatura inválida ignorada: {temp}°C")
                return None
        else:
            # Sem temperatura, dados inválidos
            return None
        
        if limite_match:
            result['limite'] = float(limite_match.group(1))
        
        if alerta_match:
            result['status_alerta'] = alerta_match.group(1)
        
        if alertas_match:
            result['alertas'] = int(alertas_match.group(1))
            
        return result
                
    except Exception as e:
        logging.error(f"Erro ao extrair dados: {e}")
        return None

# Função para obter o endereço IP local do servidor
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 1))
        local_ip = s.getsockname()[0]
        s.close()
        return local_ip
    except Exception:
        # Fallback se não conseguir determinar o IP
        return socket.gethostbyname(socket.gethostname())

# Função para processar mensagens TCP (middleware)
def process_tcp_message(message, client_address):
    """Processa mensagens TCP recebidas e atualiza dados"""
    global latest_data, message_count
    
    # Apenas processa mensagens que parecem dados de temperatura
    if "Temperatura:" in message:
        message_count += 1
        logging.info(f"Mensagem TCP #{message_count} de {client_address[0]}: '{message}'")
        
        # Extrai os dados da mensagem
        data = extract_data_from_message(message)
        
        if data and 'temperatura' in data:
            # Atualiza os dados mais recentes
            latest_data.update(data)
            
            # Envia para clientes web via WebSocket
            socketio.emit('novo_dado', data)
            logging.info(f"Dados processados e enviados via WebSocket: {data}")
            
            return True
        else:
            logging.warning(f"Dados inválidos: '{message}'")
    else:
        logging.info(f"Ignorando mensagem não reconhecida: '{message}'")
    
    return False

# Função para responder a requisições HTTP
def handle_http_request(client_socket, method, path, headers):
    """Responde a requisições HTTP na porta TCP"""
    if method == "GET" and path == "/tcp-info":
        # Endpoint especial para informações do servidor TCP
        response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"
        response += f"Servidor TCP ativo\nConexões: {connection_count}\nMensagens: {message_count}\n"
        client_socket.send(response.encode('utf-8'))
    else:
        # Resposta padrão para outras requisições HTTP
        response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"
        response += "Esta é a porta TCP/HTTP combinada. Use a interface web em http://endereço:5001/"
        client_socket.send(response.encode('utf-8'))

# Função para iniciar o servidor TCP
def run_tcp_server(stop_event=None):
    global connection_count
    local_ip = get_local_ip()
    
    logging.info(f"Iniciando servidor TCP em {local_ip}:{TCP_PORT}")
    print(f"\n[INFO] O mesmo servidor (porta {TCP_PORT}) está sendo usado para:")
    print(f"       - Servidor Web (HTTP)") 
    print(f"       - Comunicação WebSocket")
    print(f"       - Recepção de dados TCP do Pico W")
    print(f"\n[IMPORTANTE] Configure o Pico W com: SERVER_IP=\"{local_ip}\"")
    
    # Criar socket TCP
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind(('0.0.0.0', TCP_PORT))
        server_socket.listen(5)
        server_socket.settimeout(0.2)  # Timeout curto para não bloquear o eventlet
        
        while not (stop_event and stop_event.is_set()):
            try:
                # Tentar aceitar conexão
                client_socket, client_address = server_socket.accept()
                connection_count += 1
                
                # Processar esta conexão em uma thread separada para não bloquear
                thread = threading.Thread(
                    target=handle_tcp_connection, 
                    args=(client_socket, client_address)
                )
                thread.daemon = True
                thread.start()
                
            except socket.timeout:
                # Timeout normal, continuar loop
                eventlet.sleep(0)  # Cede para o eventlet
            except Exception as e:
                logging.error(f"Erro no servidor TCP: {e}")
                eventlet.sleep(1)
    
    except Exception as e:
        logging.error(f"Erro fatal no servidor TCP: {e}")
    
    finally:
        if server_socket:
            server_socket.close()
            logging.info("Servidor TCP encerrado")

# Função para tratar conexões TCP individuais
def handle_tcp_connection(client_socket, client_address):
    """Processa uma conexão individual"""
    try:
        client_socket.settimeout(3.0)  # 3 segundos timeout
        
        # Receber dados
        data = client_socket.recv(1024)
        if not data:
            return
            
        # Decodificar dados
        message = data.decode('utf-8', errors='replace').strip()
        
        # Detectar se é HTTP ou mensagem TCP simples
        if message.startswith(('GET ', 'POST ', 'PUT ', 'DELETE ', 'HEAD ')):
            # É uma requisição HTTP
            parts = message.split(' ', 2)
            if len(parts) >= 2:
                method = parts[0]
                path = parts[1]
                headers = {}
                
                # Processar como HTTP
                handle_http_request(client_socket, method, path, headers)
        else:
            # É uma mensagem TCP simples do Pico W
            if process_tcp_message(message, client_address):
                # Se processou com sucesso, enviar confirmação
                client_socket.send(f"OK: Dados recebidos às {datetime.now().strftime('%H:%M:%S')}\n".encode('utf-8'))
            else:
                client_socket.send(f"ERRO: Formato de dados não reconhecido\n")
    
    except socket.timeout:
        logging.debug(f"Timeout na conexão com {client_address[0]}")
    except Exception as e:
        logging.error(f"Erro ao processar conexão de {client_address[0]}: {e}")
    
    finally:
        client_socket.close()

# Rota principal para exibir a interface web
@app.route('/')
def index():
    return render_template('index.html')

# Rota para exibir informações sobre o servidor
@app.route('/info')
def server_info():
    local_ip = get_local_ip()
    info = {
        'ip_local': local_ip,
        'porta': TCP_PORT,
        'conexoes': connection_count,
        'mensagens': message_count,
        'ultima_temperatura': latest_data.get('temperatura'),
        'ultima_atualizacao': latest_data.get('hora_formatada', 'Nunca'),
        'codigo_pico': f'#define SERVER_IP "{local_ip}"'
    }
    return render_template('info.html', info=info)

# Rota de API para os dados mais recentes
@app.route('/api/data')
def api_data():
    return jsonify(latest_data)

# Evento Socket.IO de conexão
@socketio.on('connect')
def handle_connect():
    # Envia os dados mais recentes para um cliente que acabou de se conectar
    emit('novo_dado', latest_data)
    logging.info("Novo cliente web conectado via WebSocket")

# Iniciar o servidor TCP em uma thread separada
def initialize_tcp_thread():
    stop_event = threading.Event()
    tcp_thread = threading.Thread(target=run_tcp_server, args=(stop_event,))
    tcp_thread.daemon = True
    tcp_thread.start()
    return tcp_thread, stop_event

# Função principal
def main():
    local_ip = get_local_ip()
    
    print("\n=== SISTEMA DE MONITORAMENTO DE TEMPERATURA ===")
    print(f"Iniciando servidor em http://{local_ip}:{WEB_PORT}")
    print("Pressione Ctrl+C para encerrar\n")
    
    # Inicia o servidor TCP em uma thread separada
    tcp_thread, stop_event = initialize_tcp_thread()
    
    try:
        # IMPORTANTE: NÃO usar o socket do Flask para o servidor HTTP
        # Em vez disso, usar o socketio que já lida com isso
        socketio.run(app, host='0.0.0.0', port=WEB_PORT, debug=False, use_reloader=False)
    except KeyboardInterrupt:
        print("\nEncerrando servidor...")
    except Exception as e:
        print(f"Erro ao iniciar servidor: {e}")
    finally:
        # Ativa o evento para parar a thread do servidor TCP
        if stop_event:
            stop_event.set()
            tcp_thread.join(timeout=1.0)
        print("Servidor encerrado")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Erro ao iniciar o sistema: {e}")
        print("Falha ao iniciar o sistema")