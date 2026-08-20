#include "Server.hpp"
#include "Exceptions.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>
#include <iostream>

Server::Server(int port, const string& password) : socketFd(-1), port(port), password(password) {}

void Server::start() {
	if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		manageErrorsFromSocket(errno, __FILE__, __LINE__ - 1);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
		close(socketFd);
		manageErrorsFromBind(errno, __FILE__, __LINE__ - 2);
	}

	if (listen(socketFd, SOMAXCONN) == -1) {
		close(socketFd);
		manageErrorsFromListen(errno, __FILE__, __LINE__ - 2);
	}

	pollfd listenfd;
	listenfd.fd = socketFd;
	listenfd.events = POLLIN;
	listenfd.revents = 0;

	pollFds.push_back(listenfd);

	startMainLoop();
}

void Server::startMainLoop() {
	while (true) {
		int pollRes = poll(pollFds.data(), pollFds.size(), -1);
		if (pollRes < 0) {
			// Posibles errores de poll
		}
		for (size_t i = 0; i < pollFds.size(); i++) {
			//char* buff[1024];
			// Hay datos en espera de lectura
			if (pollFds[i].revents & POLLIN) {
				// Socket de escucha: Se debe crear un nuevo cliente
				if (pollFds[i].fd == socketFd) {
					int acceptRes = accept(socketFd, NULL, NULL);
					std::cout << "Nuevo cliente" << acceptRes << std::endl;
				}
				// Datos de lectura: Se debe leer la información de un cliente
				else {

				}
			}
			// Se puede escribir en el fd sin bloquear: Aquí se envían los mensajes
			if (pollFds[i].revents & POLLOUT) {

			}
			// Se ha cerrado la conexión: Se debe liberar el file descriptor y eliminar al usuario
			if (pollFds[i].revents & POLLHUP) {

			}
			// Error en el file descriptor: Se debe eliminar el usuario
			if (pollFds[i].revents & POLLERR) {

			}
			// File descriptor cerrado o inválido: Se debe eliminar ese cliente (Es necesario solo por precaución)
			if (pollFds[i].revents & POLLNVAL) {

			}
			// Hay datos urgentes que procesar
			if (pollFds[i].revents & POLLPRI) {

			}
		}
	}
}
