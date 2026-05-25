from dalybms import DalyBMS
import socket
import pickle

bms1 = DalyBMS()
bms1.connect('/dev/ttyUSB0')

def cliente(host='192.168.1.1', puerto=5000):
    while True:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, puerto))

        # 1. Enviar datos
        datos = bms1.get_all()
        data = pickle.dumps(datos)
        s.sendall(data)
        s.shutdown(socket.SHUT_WR)  # indica que terminó de enviar

        # 2. Esperar comando del receptor
        respuesta = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            respuesta += chunk

        comando = pickle.loads(respuesta)
        print(f"Comando recibido: {comando}")

        # 3. Ejecutar comando
        if comando == "charge_off":
            bms1.set_charge_mosfet(False)
            print("MOSFET de carga apagado")
        elif comando == "charge_on":
            bms1.set_charge_mosfet(True)
            print("MOSFET de carga encendido")
        elif comando == "ninguno":
            print("Sin comando")

        s.close()

cliente()