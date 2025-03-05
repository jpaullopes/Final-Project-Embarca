import serial
import time
import sys
import logging

# Configuração de logging
logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s - %(levelname)s - %(message)s',
                    handlers=[
                        logging.FileHandler("serial_monitor.log"),
                        logging.StreamHandler(sys.stdout)
                    ])
logger = logging.getLogger(__name__)

def monitor_serial(port='COM4', baud=115200, timeout=300):
    """
    Monitor simples de porta serial que exibe todos os dados recebidos
    e os salva em um arquivo.
    
    Args:
        port: Porta serial a ser monitorada
        baud: Taxa de transmissão
        timeout: Tempo em segundos para monitorar (0 = indefinido)
    """
    try:
        # Tenta abrir a porta serial
        ser = serial.Serial(port, baud, timeout=1)
        logger.info(f"Porta {port} aberta com baud rate {baud}")
        
        # Arquivo para salvar os dados brutos
        raw_file = open("serial_raw.txt", "wb")
        
        start_time = time.time()
        bytes_received = 0
        lines_received = 0
        
        logger.info("Monitoramento iniciado. Pressione Ctrl+C para encerrar.")
        
        # Loop de monitoramento
        while timeout == 0 or (time.time() - start_time) < timeout:
            try:
                if ser.in_waiting > 0:
                    # Lê os dados disponíveis
                    data = ser.read(ser.in_waiting)
                    bytes_received += len(data)
                    
                    # Salva os dados brutos
                    raw_file.write(data)
                    raw_file.flush()
                    
                    # Tenta decodificar como texto
                    try:
                        text = data.decode('utf-8')
                        # Conta as linhas
                        lines_received += text.count('\n')
                        # Mostra no console
                        print(text, end='')
                    except UnicodeDecodeError:
                        # Se não conseguir decodificar, mostra os bytes
                        hex_data = ' '.join([f"{b:02x}" for b in data])
                        logger.warning(f"Dados não-UTF8: {hex_data}")
                
                # Pequena pausa para não sobrecarregar a CPU
                time.sleep(0.01)
                
            except KeyboardInterrupt:
                logger.info("Monitoramento interrompido pelo usuário.")
                break
        
        # Estatísticas finais
        duration = time.time() - start_time
        logger.info(f"Monitoramento concluído após {duration:.1f} segundos")
        logger.info(f"Recebidos {bytes_received} bytes, {lines_received} linhas")
        logger.info(f"Taxa média: {bytes_received/duration:.1f} bytes/s, {lines_received/duration:.1f} linhas/s")
        
    except serial.SerialException as e:
        logger.error(f"Erro ao abrir porta serial: {e}")
    except Exception as e:
        logger.error(f"Erro não esperado: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            logger.info(f"Porta {port} fechada")
        if 'raw_file' in locals():
            raw_file.close()

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Monitor de porta serial")
    parser.add_argument('-p', '--port', default='COM4', help='Porta serial (ex: COM4)')
    parser.add_argument('-b', '--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('-t', '--timeout', type=int, default=30, help='Tempo de monitoramento em segundos (0=indefinido)')
    
    args = parser.parse_args()
    monitor_serial(args.port, args.baud, args.timeout)
