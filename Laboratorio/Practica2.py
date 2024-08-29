#Prueba comuicacion TCP
import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('localhost', 10000)
print('Iniciando {} en el puerto {}'.format(*server_address))
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print("Esperando la coneccion")
    connection, client_address = sock.accept()
    try:
        print('Conectado en', client_address)

        # Receive the data in small chunks and retransmit it
        while True:
            data = connection.recv(16)
            print('Recibido {!r}'.format(data))
            if data:
                print('Mandando data de vuelta al cliente')
                connection.sendall(data)
            else:
                print('No data de', client_address)
                break

    finally:
        # Clean up the connection
        connection.close()

