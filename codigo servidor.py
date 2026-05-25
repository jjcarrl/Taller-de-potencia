import socket
import pickle

def servidor(host='0.0.0.0', puerto=5000):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, puerto))
    s.listen(1)
    print(f"Esperando conexión en puerto {puerto}...")

    while True:
        conn, addr = s.accept()
        print(f"Conectado desde {addr}")

        # 1. Recibir datos
        data = b""
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                break
            data += chunk

        datos = pickle.loads(data)
        print(f"Datos recibidos: {datos}")

        # 2. Decidir comando
        comando = input("Escribe comando (charge_off / charge_on / ninguno): ")

        # 3. Enviar comando
        conn.sendall(pickle.dumps(comando))
        conn.close()

servidor()