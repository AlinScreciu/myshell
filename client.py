import socket 
import time
port = int(input("What port would you want? "))

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client:
	client.connect(('127.0.0.1',port))
	while True:
		command = input("Command:\n")
		command = command.strip()
		client.send(str(len(command)).encode())
		time.sleep(0.05);
		client.send(command.encode())
		if (command == "exit"):
			break
		a = client.recv(1000).decode()
		time.sleep(0.1)
		b = client.recv(int(a)).decode()
		print(a,b)
		