#Prueba comuicacion UDP

import socket
from time import sleep

HOST = 'localhost'
#IP = "192.168.0.194"
#PORT = 52545

IP = "148.231.130.229" 
PORT = 22
MESSAGE = "HELLO"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(5)

print(f"Mandando Informacion")
print(f"{MESSAGE} to {IP}:{PORT}")
sock.sendto(MESSAGE.encode(), (IP, PORT))

try:
    print(f"Escuchando de {IP}:{PORT}")
    #data = sock.recvfrom(1024)
    data = sock.recv(1024)
    print(f"received message: {data}")
except socket.error:
    print("No respondio")


#while(True):
    #print(f"Mandando Informacion")
    #print(f"{MESSAGE} to {IP}:{PORT}")
    #sock.sendto(MESSAGE.encode(), (IP, PORT))

    #try:
        #print(f"Escuchando de {IP}:{PORT}")
        #data = sock.recvfrom(1024)
        #print(f"received message: {data}")
    #except socket.error:
        #print("No respondio")

    #sleep(2)


