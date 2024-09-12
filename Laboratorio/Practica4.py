#Buscador de esp-32
import socket
import time

# Configuración
PORT = 3333           # Puerto en el que está escuchando el ESP32
message = "UABC:IP"  # Mensaje inicial para identificar la IP del ESP32

def send_broadcast(PORT, message):
    for i in range(1, 255):
        ip = f"192.168.4.{i}"
        print(f"Intentando enviar mensaje a {ip}:{PORT}")
        
        # Crear socket UDP
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(1)  # Tiempo de espera de 2 segundos para la respuesta

        try:
            # Enviar mensaje
            sock.sendto(message.encode(), (ip, PORT))

            # Intentar recibir respuesta
            data, addr = sock.recvfrom(1024)
            print(f"Respuesta recibida de {addr[0]}: {data.decode()}")
            return addr[0]  # Retorna la IP que respondió
        except socket.timeout:
            print(f"No hay respuesta de {ip}")
        except ConnectionResetError:
            print(f"Conexión reiniciada por {ip}, ignorando y continuando.")
        except socket.error as e:
            print(f"Error en el socket con {ip}: {e}")
        finally:
            sock.close()

    print("No se encontró ningún dispositivo que respondiera.")

# Enviar mensaje de broadcast y recibir respuestas
IP = send_broadcast(PORT, message)
print("IP: " + str(IP))

# Pedir al usuario que ingrese el mensaje
while True:
    MESSAGE = input("Por favor, ingresa el mensaje que deseas enviar: ")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(10)

    print(f"Mandando Información")
    print(f"{MESSAGE} to {IP}:{PORT}")
    sock.sendto(MESSAGE.encode(), (IP, PORT))

    try:
        print(f"Escuchando de {IP}:{PORT}")
        data = sock.recv(1024)
        print(f"Mensaje recibido: {data.decode()}")
    except socket.error:
        print("No respondió")

