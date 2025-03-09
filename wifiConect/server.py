import socket
import json
from datetime import datetime
import ipaddress

# Server configuration
HOST = ''  # Listen on all interfaces
PORT = 12345  # Use the same port as defined in Pico W code

def get_all_ip_addresses():
    """Obter todos os endereços IP das interfaces de rede disponíveis"""
    hostname = socket.gethostname()
    ip_list = {}
    
    # Obter endereços IPv4
    try:
        ip_list[hostname] = socket.gethostbyname(hostname)
    except:
        pass
        
    # Obter todos os endereços IP
    try:
        for info in socket.getaddrinfo(hostname, None):
            ip = info[4][0]
            # Filtrar apenas endereços IPv4 que não são loopback
            try:
                ipobj = ipaddress.ip_address(ip)
                if ipobj.version == 4 and not ipobj.is_loopback:
                    ip_list[info[0]] = ip
            except:
                pass
    except:
        pass
        
    return ip_list

def main():
    print(f"===== Raspberry Pi Pico W - TCP Server =====")
    print(f"Starting server on port {PORT}...")
    
    # Mostrar todos os endereços IP disponíveis
    ip_addresses = get_all_ip_addresses()
    print("\nEndereços IP disponíveis:")
    for name, ip in ip_addresses.items():
        print(f"- {ip}")
    print("\nO Pico W deve conectar-se a um desses endereços IP.")
    
    # Create a TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Allow port reuse (helpful for development)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen()
        print(f"\nServer escutando em todas as interfaces na porta {PORT}")
        print("Aguardando conexões...")
        
        try:
            while True:
                # Accept connection
                conn, addr = s.accept()
                with conn:
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    print(f"\n[{timestamp}] Conectado a: {addr[0]}:{addr[1]}")
                    
                    # Receive data
                    data = conn.recv(1024)
                    if not data:
                        continue
                    
                    # Print received data
                    data_str = data.decode('utf-8')
                    print(f"Dados recebidos: {data_str}")
                    
                    # Try to parse JSON (our data is in a JSON-like format)
                    try:
                        # Replace single quotes with double quotes for proper JSON
                        json_str = data_str.replace("'", '"')
                        json_data = json.loads(json_str)
                        print(f"Temperatura: {json_data.get('temperature', 'N/A')}°C")
                        print(f"Umidade: {json_data.get('humidity', 'N/A')}%")
                    except json.JSONDecodeError:
                        print("Não foi possível analisar os dados JSON")
                    
                    # Send acknowledgment back to client
                    conn.sendall(b"Dados recebidos com sucesso")
                    
        except KeyboardInterrupt:
            print("\nServidor interrompido pelo usuário")
        except Exception as e:
            print(f"\nErro: {e}")

if __name__ == "__main__":
    main()