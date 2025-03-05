"""
Script para testar a comunicação serial com o dispositivo.
Execute este script separadamente para verificar se os dados 
estão chegando corretamente da porta serial.
"""
import serial
import time
import argparse

def test_serial(port='COM4', baud=115200, duration=30):
    """
    Testa a comunicação serial, lendo dados por um período específico.
    Args:
        port: Porta serial (ex: COM4, /dev/ttyACM0)
        baud: Taxa de baud (velocidade)
        duration: Duração do teste em segundos
    """
    try:
        # Tenta abrir a porta serial
        ser = serial.Serial(port, baud, timeout=1)
        print(f"Porta {port} aberta com sucesso a {baud} baud")
        
        # Configuração para leitura
        print(f"Iniciando leitura por {duration} segundos. Pressione Ctrl+C para interromper.")
        print("-" * 50)
        
        start_time = time.time()
        line_count = 0
        
        # Lê dados por 'duration' segundos
        while time.time() - start_time < duration:
            if ser.in_waiting > 0:
                try:
                    # Lê uma linha de dados
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    timestamp = time.strftime("%H:%M:%S")
                    
                    # Mostra os dados recebidos
                    print(f"[{timestamp}] Recebido: '{line}'")
                    
                    # Tenta parsar os dados como esperado pelo app.py
                    parts = line.split()
                    if len(parts) >= 2:
                        try:
                            intensity = int(float(parts[0]))
                            value = float(parts[1])
                            print(f"   ✓ Decodificado: intensidade={intensity}, valor={value}")
                        except ValueError as e:
                            print(f"   ✗ Erro ao converter valores: {e}")
                    else:
                        print(f"   ✗ Formato inválido: esperado 2 valores, recebido {len(parts)}")
                    
                    line_count += 1
                except UnicodeDecodeError as e:
                    print(f"[{timestamp}] Erro de decodificação: {e}")
            
            # Pequeno delay para não sobrecarregar a CPU
            time.sleep(0.01)
        
        # Resumo
        print("-" * 50)
        print(f"Teste concluído. {line_count} linhas recebidas em {duration} segundos.")
        print(f"Taxa média: {line_count/duration:.2f} linhas por segundo")
        
    except serial.SerialException as e:
        print(f"Erro ao abrir porta {port}: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Porta {port} fechada")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Teste de comunicação serial")
    parser.add_argument('-p', '--port', default='COM4', help='Porta serial (ex: COM4)')
    parser.add_argument('-b', '--baud', type=int, default=115200, help='Taxa de baud')
    parser.add_argument('-d', '--duration', type=int, default=30, help='Duração do teste em segundos')
    
    args = parser.parse_args()
    test_serial(args.port, args.baud, args.duration)
