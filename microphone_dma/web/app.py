from flask import Flask, render_template, jsonify, current_app
import serial
import time
import threading
import queue
import random  # Para modo de simulação
import logging  # Para melhor logging
import atexit
import os
import sys

# Configurar logging
logging.basicConfig(level=logging.WARNING,  # Mudar de INFO para WARNING para reduzir logs no terminal
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__)

# Configuração
SIMULATION_MODE = False  # Altere para True para simular dados sem usar porta serial
SERIAL_PORT = 'COM4'
BAUD_RATE = 115200
USE_RELOADER = False  # Desabilitar o reloader para evitar problemas com a porta serial

# Ativar modo de depuração detalhada
DEBUG_SERIAL = True

# Buffer para guardar dados brutos recebidos (para diagnóstico)
raw_buffer = []
MAX_RAW_BUFFER = 100  # Limita quantidade de dados armazenados

# Fila para armazenar os dados mais recentes
data_queue = queue.Queue(maxsize=100)
latest_data = {"intensity": 0, "value": 0.0, "timestamp": time.time()}
should_run = True
connection_status = {"connected": False, "last_error": "Inicializando..."}

# Objeto serial global para facilitar o fechamento no encerramento
serial_instance = None

# Adicionar novas configurações
LOG_DATA_POINTS = False  # Controla se os dados são logados no console
SMOOTHING_WINDOW = 5     # Tamanho da janela para média móvel
smoothing_buffer = []    # Buffer para suavizar valores

# Configuração da porta serial
def setup_serial():
    global serial_instance
    try:
        # Verifica se já existe uma instância aberta e fecha
        if serial_instance:
            try:
                serial_instance.close()
                logger.info("Fechando conexão serial anterior")
            except:
                pass
            
        # Tenta abrir a porta serial
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        serial_instance = ser
        logger.info(f"Porta serial {SERIAL_PORT} aberta com sucesso")
        connection_status["connected"] = True
        connection_status["last_error"] = ""
        return ser
    except serial.SerialException as e:
        error_msg = f"Erro ao abrir porta {SERIAL_PORT}: {str(e)}"
        logger.error(error_msg)
        connection_status["connected"] = False
        connection_status["last_error"] = error_msg
        return None

# Função para limpar recursos no encerramento
def cleanup_resources():
    global should_run, serial_instance
    logger.info("Limpando recursos...")
    should_run = False
    if serial_instance:
        try:
            serial_instance.close()
            logger.info("Porta serial fechada no encerramento")
        except:
            pass

# Registra a função de limpeza para ser chamada no encerramento
atexit.register(cleanup_resources)

def simulate_data():
    """Gera dados simulados para testes"""
    while should_run:
        # Simula um valor entre 0 e 3.3V com intensidade de 0 a 5
        value = random.uniform(0, 3.3)
        intensity = min(5, int(value * 1.5))  # Escala para 0-5
        
        data_point = {
            "intensity": intensity,
            "value": value,
            "timestamp": time.time()
        }
        
        logger.debug(f"Dados simulados: {data_point}")
        global latest_data
        latest_data = data_point
        
        # Adiciona à fila, remove o mais antigo se estiver cheia
        if data_queue.full():
            data_queue.get()
        data_queue.put(data_point)
        
        time.sleep(0.2)  # Simula uma atualização a cada 200ms

# Thread para ler os dados da porta serial
def read_serial_data():
    global latest_data, should_run, connection_status, serial_instance, SIMULATION_MODE, raw_buffer, smoothing_buffer
    
    if SIMULATION_MODE:
        logger.info("Iniciando modo de SIMULAÇÃO")
        simulate_data()
        return

    # Se estamos no modo debug com reloader, o processo filho não deve tentar usar a porta serial
    if os.environ.get('WERKZEUG_RUN_MAIN') != 'true' and sys.argv[0].endswith('flask'):
        logger.info("Processo pai detectado em modo reloader, ignorando inicialização serial")
        return
    
    ser = setup_serial()
    retry_count = 0
    max_retries = 5
    
    if not ser:
        logger.warning(f"Falha ao abrir porta serial. Tentando novamente em 5 segundos...")
        time.sleep(5)
        ser = setup_serial()
    
    if not ser:
        logger.error("Não foi possível abrir a porta serial após tentativas. Verifique a conexão.")
        connection_status["last_error"] = "Falha ao conectar com o dispositivo"
        # Ativa o modo de simulação se falhar na conexão com a porta
        if app.config.get('AUTO_SWITCH_TO_SIMULATION', True):
            logger.warning("Ativando modo de simulação automaticamente devido a falha de conexão")
            SIMULATION_MODE = True
            simulate_data()
        return
    
    logger.info("Iniciando leitura da porta serial...")
    
    while should_run:
        try:
            if ser.in_waiting > 0:
                # Lê os bytes disponíveis (preserva dados brutos)
                raw_data = ser.read(ser.in_waiting)
                
                if DEBUG_SERIAL:
                    # Log dos bytes brutos
                    hex_data = ' '.join([f"{b:02x}" for b in raw_data])
                    logger.debug(f"Bytes recebidos: {hex_data}")
                    
                    # Guarda no buffer de diagnóstico
                    timestamp = time.time()
                    raw_buffer.append({
                        "time": timestamp,
                        "data_hex": hex_data,
                        "data_str": raw_data.decode('utf-8', errors='replace')
                    })
                    
                    # Limita o tamanho do buffer
                    if len(raw_buffer) > MAX_RAW_BUFFER:
                        raw_buffer.pop(0)
                
                # Tenta decodificar e processar as linhas
                text = raw_data.decode('utf-8', errors='replace')
                lines = text.splitlines()
                
                for line in lines:
                    if not line.strip():
                        continue  # Ignora linhas vazias
                    
                    # Ignora linhas de DEBUG do microcontrolador
                    if line.startswith("DEBUG:"):
                        continue
                        
                    # Mudança de INFO para DEBUG para reduzir logs no terminal
                    if LOG_DATA_POINTS:
                        logger.debug(f"Linha recebida: '{line}'")
                    
                    # O formato esperado é: "XX X.XXXX" (intensidade e valor)
                    parts = line.split()
                    
                    # Tenta extrair valores mesmo com formato irregular
                    if len(parts) != 2:
                        # Tenta extrair números da string
                        numbers = []
                        current_number = ""
                        for char in line:
                            if char.isdigit() or char == '.':
                                current_number += char
                            elif current_number:
                                numbers.append(current_number)
                                current_number = ""
                        
                        if current_number:
                            numbers.append(current_number)
                            
                        logger.info(f"Extração de números: {numbers}")
                        
                        if len(numbers) >= 2:
                            parts = numbers[:2]
                            logger.info(f"Usando valores extraídos: {parts}")
                    
                    if len(parts) >= 2:
                        try:
                            intensity = int(float(parts[0]))
                            value = float(parts[1])
                            
                            # Aplicar média móvel para suavizar as leituras
                            smoothing_buffer.append(value)
                            if len(smoothing_buffer) > SMOOTHING_WINDOW:
                                smoothing_buffer.pop(0)
                            
                            smoothed_value = sum(smoothing_buffer) / len(smoothing_buffer)
                            
                            # Log reduzido e mudado para DEBUG
                            if LOG_DATA_POINTS:
                                logger.debug(f"Processado: i={intensity}, v={smoothed_value:.4f} (orig={value:.4f})")
                            
                            # Cria o ponto de dados com valor suavizado
                            data_point = {
                                "intensity": intensity,
                                "value": smoothed_value,
                                "raw_value": value,  # Mantém o valor original para referência
                                "timestamp": time.time()
                            }
                            
                            # Atualiza dados globais
                            latest_data = data_point
                            
                            # Adiciona à fila
                            if data_queue.full():
                                data_queue.get()
                            data_queue.put(data_point)
                            
                            retry_count = 0
                        except ValueError as e:
                            if LOG_DATA_POINTS:
                                logger.warning(f"Erro de conversão: '{line}' - {e}")
                    else:
                        if LOG_DATA_POINTS:
                            logger.warning(f"Formato inválido: '{line}' - partes: {parts}")
            else:
                time.sleep(0.01)
                
        except Exception as e:
            retry_count += 1
            error_msg = f"Erro na leitura serial: {e}"
            logger.error(error_msg)
            connection_status["last_error"] = error_msg
            
            if retry_count >= max_retries:
                logger.error("Número máximo de tentativas excedido, reconectando...")
                if ser:
                    try:
                        ser.close()
                    except:
                        pass
                time.sleep(2)
                ser = setup_serial()
                retry_count = 0
                if not ser:
                    logger.error("Falha ao reconectar. Nova tentativa em 5 segundos.")
                    time.sleep(5)
                    ser = setup_serial()
                    if not ser:
                        logger.error("Falha persistente na conexão serial.")
                        break
            
            time.sleep(1)
    
    if ser:
        try:
            ser.close()
            serial_instance = None
            logger.info("Porta serial fechada")
        except:
            pass

# Inicia a thread de leitura serial
serial_thread = threading.Thread(target=read_serial_data)
serial_thread.daemon = True
serial_thread.start()

@app.route('/')
def index():
    # Passa a configuração SIMULATION_MODE para o template
    app.logger.info(f"Renderizando página inicial com simulation_mode={SIMULATION_MODE}")
    return render_template('index.html', simulation_mode=SIMULATION_MODE)

@app.route('/api/data')
def get_data():
    # Converte a fila em uma lista para o JSON
    data_list = list(data_queue.queue)
    
    # Log detalhado para depuração
    app.logger.info(f"API /data: Enviando {len(data_list)} registros históricos, último: {latest_data}")
    
    return jsonify({
        "latest": latest_data,
        "history": data_list,
        "status": connection_status,
        "queue_size": data_queue.qsize(),  # Adicionar informação do tamanho da fila
        "sim_mode": SIMULATION_MODE        # Status do modo de simulação
    })

@app.route('/api/latest')
def get_latest():
    return jsonify({
        "data": latest_data,
        "status": connection_status
    })

@app.route('/api/status')
def get_status():
    return jsonify(connection_status)

# Rota para alternar o modo de simulação
@app.route('/api/toggle_simulation')
def toggle_simulation():
    global SIMULATION_MODE, serial_thread, should_run
    
    # Inverter o modo
    SIMULATION_MODE = not SIMULATION_MODE
    app.logger.info(f"Modo de simulação alterado para: {SIMULATION_MODE}")
    
    # Atualiza a configuração da aplicação
    app.config['SIMULATION_MODE'] = SIMULATION_MODE
    
    # Se estiver ativando o modo de simulação
    if SIMULATION_MODE:
        logger.info("Ativando modo de simulação")
        # Inicia uma nova thread de simulação se necessário
        if not any(t.name == 'simulation_thread' for t in threading.enumerate()):
            sim_thread = threading.Thread(target=simulate_data, name='simulation_thread')
            sim_thread.daemon = True
            sim_thread.start()
    else:
        logger.info("Desativando modo de simulação")
        # Se desativar a simulação, limpar a fila e tentar reconectar à porta serial
        if not connection_status["connected"]:
            data_queue.queue.clear()  # Limpa a fila de dados
            # Iniciar thread para tentar reconectar à porta serial
            reconnect_thread = threading.Thread(target=read_serial_data)
            reconnect_thread.daemon = True
            reconnect_thread.start()
    
    return jsonify({
        "simulation_mode": SIMULATION_MODE,
        "message": f"Modo de simulação {'ativado' if SIMULATION_MODE else 'desativado'}"
    })

# Adiciona endpoint para diagnóstico
@app.route('/api/diagnostic')
def get_diagnostic():
    return jsonify({
        "raw_buffer": raw_buffer,
        "latest_data": latest_data,
        "queue_size": data_queue.qsize(),
        "simulation_mode": SIMULATION_MODE,
        "connection": connection_status,
        "app_info": {
            "time": time.time(),
            "debug_serial": DEBUG_SERIAL,
            "serial_port": SERIAL_PORT,
            "baud_rate": BAUD_RATE
        }
    })

if __name__ == '__main__':
    try:
        # Configuração adicional da aplicação
        app.config['AUTO_SWITCH_TO_SIMULATION'] = True
        app.config['SIMULATION_MODE'] = SIMULATION_MODE
        
        logger.info(f"Iniciando servidor Flask no modo {'SIMULAÇÃO' if SIMULATION_MODE else 'NORMAL'}")
        
        # Desabilitar reloader para evitar problemas com a porta serial
        app.run(debug=True, host='0.0.0.0', port=5000, use_reloader=USE_RELOADER)
    finally:
        should_run = False
        logger.info("Encerrando thread de leitura serial...")
        cleanup_resources()
        if serial_thread.is_alive():
            serial_thread.join(timeout=2)
