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

# IMPORTANTE: Usando portas diferentes para TCP e web
TCP_PORT = 5001   # Porta para o Pico W se conectar (não mude, já está configurado no firmware)
WEB_PORT = 8080   # Porta para a interface web (mudamos para evitar conflito)

#variaveis globais
temperature_history = []  # Histórico de temperaturas para estatísticas
min_temp = 999.0          # Temperatura mínima registrada
max_temp = -999.0         # Temperatura máxima registrada
avg_temp = 0.0            # Temperatura média

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
    global latest_data, message_count, temperature_history, min_temp, max_temp, avg_temp
    
    # Apenas processa mensagens que parecem dados de temperatura
    if "Temperatura:" in message:
        message_count += 1
        logging.info(f"Mensagem TCP #{message_count} de {client_address[0]}: '{message}'")
        
        # Extrai os dados da mensagem
        data = extract_data_from_message(message)
        
        if data and 'temperatura' in data:
            # Obtém a temperatura atual
            current_temp = data['temperatura']
            
            # Atualiza estatísticas
            temperature_history.append(current_temp)
            if len(temperature_history) > 1000:  # Limita o tamanho do histórico
                temperature_history = temperature_history[-1000:]
                
            # Atualiza estatísticas
            min_temp = min(min_temp, current_temp)
            max_temp = max(max_temp, current_temp)
            avg_temp = sum(temperature_history) / len(temperature_history)
            
            # Adiciona estatísticas aos dados
            data['min_temp'] = min_temp
            data['max_temp'] = max_temp
            data['avg_temp'] = avg_temp
            
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
        
        # Processar como mensagem TCP do Pico W
        if process_tcp_message(message, client_address):
            # Se processou com sucesso, enviar confirmação
            client_socket.send(f"OK: Dados recebidos às {datetime.now().strftime('%H:%M:%S')}\n".encode('utf-8'))
        else:
            client_socket.send(b"ERRO: Formato de dados nao reconhecido\n")
    
    except socket.timeout:
        logging.debug(f"Timeout na conexão com {client_address[0]}")
    except Exception as e:
        logging.error(f"Erro ao processar conexão de {client_address[0]}: {e}")
    
    finally:
        client_socket.close()

# Função para iniciar o servidor TCP dedicado
def run_tcp_server(stop_event=None):
    global connection_count
    local_ip = get_local_ip()
    
    logging.info(f"Iniciando servidor TCP em {local_ip}:{TCP_PORT}")
    print(f"\n[INFO] Servidor dividido em duas portas:")
    print(f"       - Servidor TCP (Pico W): {TCP_PORT}")
    print(f"       - Interface Web: {WEB_PORT}")
    print(f"\n[IMPORTANTE] Configure o Pico W com:")
    print(f"#define SERVER_IP \"{local_ip}\"")
    print(f"#define TCP_PORT {TCP_PORT}")
    
    # Criar socket TCP dedicado
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind(('0.0.0.0', TCP_PORT))
        server_socket.listen(5)  # Aumentado para 5 conexões pendentes
        server_socket.settimeout(1.0)
        
        logging.info(f"Servidor TCP pronto para receber dados do Pico W")
        
        while not (stop_event and stop_event.is_set()):
            try:
                client_socket, client_address = server_socket.accept()
                connection_count += 1
                
                thread = threading.Thread(
                    target=handle_tcp_connection, 
                    args=(client_socket, client_address)
                )
                thread.daemon = True
                thread.start()
                
            except socket.timeout:
                pass  # Timeout normal
            except Exception as e:
                logging.error(f"Erro no servidor TCP: {e}")
                time.sleep(1)
    
    except Exception as e:
        logging.error(f"Erro ao iniciar servidor TCP: {e}")
    
    finally:
        if server_socket:
            server_socket.close()
            logging.info("Servidor TCP encerrado")

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
        'porta_tcp': TCP_PORT,
        'porta_web': WEB_PORT,
        'conexoes': connection_count,
        'mensagens': message_count,
        'ultima_temperatura': latest_data.get('temperatura'),
        'ultima_atualizacao': latest_data.get('hora_formatada', 'Nunca'),
        'codigo_pico': f'#define SERVER_IP "{local_ip}"\n#define TCP_PORT {TCP_PORT}'
    }
    return render_template('info.html', info=info)

# Rota de API para os dados mais recentes
@app.route('/api/data')
def api_data():
    data = dict(latest_data)
    data['min_temp'] = min_temp
    data['max_temp'] = max_temp  
    data['avg_temp'] = avg_temp
    return jsonify(data)

# Evento Socket.IO de conexão
@socketio.on('connect')
def handle_connect():
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
    print(f"Iniciando servidor web em http://{local_ip}:{WEB_PORT}")
    print("Pressione Ctrl+C para encerrar\n")
    
    # Inicia o servidor TCP em uma thread separada
    tcp_thread, stop_event = initialize_tcp_thread()
    
    try:
        # Inicia a interface web em uma porta diferente
        socketio.run(app, host='0.0.0.0', port=WEB_PORT, debug=False, use_reloader=False)
    except KeyboardInterrupt:
        print("\nEncerrando servidor...")
    except Exception as e:
        print(f"Erro ao iniciar servidor web: {e}")
    finally:
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