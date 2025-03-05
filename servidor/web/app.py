import eventlet
eventlet.monkey_patch()

from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import serial.tools.list_ports
from serial import Serial, SerialException
import threading
import re
import time
import logging

# Configuração de logging para reduzir saída no console
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(levelname)s - %(message)s',
                    datefmt='%H:%M:%S')

# Desativar logs de werkzeug (Flask) para manter o console limpo
logging.getLogger('werkzeug').setLevel(logging.WARNING)
logging.getLogger('engineio').setLevel(logging.WARNING)
logging.getLogger('socketio').setLevel(logging.WARNING)

# Criação do aplicativo Flask
app = Flask(__name__)
socketio = SocketIO(app, async_mode='eventlet')

# Flag para controlar o nível de verbosidade dos logs
VERBOSE_LOGGING = False

# Função para extrair dados completos da mensagem
def extract_data_from_serial(data):
    """Extrai temperatura, limite, status de alerta e contador de alertas da mensagem serial"""
    try:
        # Padrão para extrair todos os dados da placa:
        # Exemplo: "Temperatura: 33.64 C | Limite: 40.0 C | Alerta: Inativo | Alertas: 1"
        temp_match = re.search(r'Temperatura: ([0-9]+(?:\.[0-9]+)?)', data)
        limite_match = re.search(r'Limite: ([0-9]+(?:\.[0-9]+)?)', data)
        alerta_match = re.search(r'Alerta: (\w+)', data)
        alertas_match = re.search(r'Alertas: ([0-9]+)', data)
        
        result = {}
        
        if temp_match:
            result['temperatura'] = float(temp_match.group(1))
        
        if limite_match:
            result['limite'] = float(limite_match.group(1))
        
        if alerta_match:
            result['status_alerta'] = alerta_match.group(1)
        
        if alertas_match:
            result['alertas'] = int(alertas_match.group(1))
            
        if 'temperatura' in result:  # Se pelo menos temos a temperatura
            return result
            
        # Fallback: tentar buscar apenas um número para a temperatura
        if not result:
            simple_match = re.search(r'([0-9]+(?:\.[0-9]+)?)', data)
            if simple_match:
                return {'temperatura': float(simple_match.group(1))}
                
        return None
    except Exception as e:
        logging.error(f"Erro ao extrair dados: {e}")
        return None

# Função para encontrar a porta serial automaticamente
def find_arduino_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        # Pode ajustar este filtro para o seu dispositivo específico
        # Ex: Arduino geralmente contém "Arduino" ou "CH340" na descrição
        if 'Arduino' in p.description or 'CH340' in p.description:
            return p.device
    
    # Se não encontrar especificamente, retorne a primeira porta disponível
    if ports:
        return ports[0].device
    
    return None

# Função para ler dados da porta serial
def read_serial(stop_event=None):
    # Tenta encontrar a porta automaticamente
    port = find_arduino_port() or 'COM4'  # Usa COM4 como fallback
    
    logging.info(f"Iniciando conexão com a porta serial: {port}")
    
    while True:
        if stop_event and stop_event.is_set():
            logging.info("Parando leitura serial por solicitação")
            break
        
        try:
            with Serial(port, 115200, timeout=1) as ser:
                logging.info(f"Conectado com sucesso à porta {port}")
                
                # Contador para limitar logs
                read_count = 0
                
                while stop_event is None or not stop_event.is_set():
                    try:
                        data = ser.readline().decode('utf-8').strip()
                        if data:
                            read_count += 1
                            
                            # Log limitado
                            if VERBOSE_LOGGING or read_count % 10 == 0:
                                logging.debug(f"Serial: {data}")
                            
                            # Extrair todos os dados disponíveis
                            parsed_data = extract_data_from_serial(data)
                            if parsed_data:
                                try:
                                    # Enviar dados completos para o cliente
                                    socketio.emit('novo_dado', parsed_data)
                                except Exception as emitError:
                                    logging.error(f"Erro ao enviar dados: {emitError}")
                            elif VERBOSE_LOGGING:
                                logging.warning(f"Erro ao converter dado: {data}")
                    except UnicodeDecodeError:
                        if VERBOSE_LOGGING:
                            logging.warning("Erro ao decodificar dados da serial")
                    # Pequena pausa para não sobrecarregar
                    time.sleep(0.01)  
        
        except SerialException as e:
            logging.error(f"Erro na conexão serial: {e}")
            logging.info("Tentando reconectar em 5 segundos...")
            time.sleep(5)
        except Exception as e:
            logging.error(f"Erro inesperado: {e}")
            time.sleep(5)

# Rota principal para exibir a interface web
@app.route('/')
def index():
    return render_template('index.html')

# Iniciar a thread de leitura serial sem bloquear o servidor web
def initialize_serial_thread():
    stop_event = threading.Event()
    serial_thread = threading.Thread(target=read_serial, args=(stop_event,))
    serial_thread.daemon = True
    serial_thread.start()
    return serial_thread, stop_event

# Função principal
def main():
    print("\n=== SISTEMA DE MONITORAMENTO DE TEMPERATURA ===")
    print("Iniciando servidor web em http://localhost:5000")
    print("Pressione Ctrl+C para encerrar\n")
    
    # Inicia a leitura serial em uma thread separada
    serial_thread, stop_event = initialize_serial_thread()
    
    try:
        socketio.run(app, host='0.0.0.0', port=5000, debug=False, use_reloader=False)
    except KeyboardInterrupt:
        print("\nEncerrando servidor...")
    except Exception as e:
        print(f"Erro ao iniciar servidor: {e}")
    finally:
        # Ativa o evento para parar a thread de leitura serial
        if stop_event:
            stop_event.set()
            serial_thread.join(timeout=1.0)
        print("Servidor encerrado")

if __name__ == "__main__":
    try:
        # Tente com threading
        main()
    except Exception as e:
        print(f"Erro ao iniciar o servidor: {e}")
        print("Falha ao iniciar o sistema")