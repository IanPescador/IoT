#Prueba tcp envio y recepcion

import socket

#HOST = '192.168.4.88'
#PORT = 3333

HOST = "iot-uabc.site"
PORT = 8080

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # Conectar al servidor
        sock.connect((HOST, PORT))
        print(f"Conectado a {HOST}:{PORT}")

        sock.settimeout(10)

        while True:
            MESSAGE = input("Por favor, ingresa el mensaje que deseas enviar: ")
            
            # Enviar mensaje
            print(f"Enviando mensaje: {MESSAGE}")
            sock.sendall(MESSAGE.encode())

            # Esperar y recibir respuesta
            data = sock.recv(1024)
            print(f"Mensaje recibido: {data.decode()}")
            
    except socket.error as e:
        print(f"Error de socket: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()