#Prueba comunicacion UDP

import socket
from time import sleep

HOST = 'localhost'
#IP = "192.168.4.88"
#IP = "192.168.4.1"
#PORT = 8266

#IP = "iot-uabc.site"
#PORT = 2807 #UDP
#PORT = 2877 #Command

IP = "192.168.0.181" #SMS direccion
PORT = 21 #UDP
MESSAGE = "UABC:IPR:W:SMS:6641896966:SILKSONG"

#Mandar un solo mensaje y esperar respuesta
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

# Pedir al usuario que ingrese el mensaje repetir ciclo
# while True:
#     MESSAGE = input("Por favor, ingresa el mensaje que deseas enviar: ")

#     sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#     sock.settimeout(10)

#     print(f"Mandando Información")
#     print(f"{MESSAGE} to {IP}:{PORT}")
#     sock.sendto(MESSAGE.encode(), (IP, PORT))

#     try:
#         print(f"Escuchando de {IP}:{PORT}")
#         data = sock.recv(1024)
#         print(f"Mensaje recibido: {data.decode()}")
#     except socket.error:
#         print("No respondió")

# Si deseas enviar mensajes continuamente y recibir respuestas, puedes descomentar el siguiente bloque
# while True:
#     print(f"Mandando Información")
#     print(f"{MESSAGE} to {IP}:{PORT}")
#     sock.sendto(MESSAGE.encode(), (IP, PORT))

#     try:
#         print(f"Escuchando de {IP}:{PORT}")
#         data = sock.recv(1024)
#         print(f"Mensaje recibido: {data.decode()}")
#     except socket.error:
#         print("No respondió")

#     sleep(2)
