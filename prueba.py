import socket, time

s = socket.socket()
s.connect(('127.0.0.1', 6667))
s.send(b"NICK test\r\nUSER test 0 * :test\r\n")
time.sleep(1)

# Cierre limpio → TCP FIN → POLLHUP en el servidor
s.close()