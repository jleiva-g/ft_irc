#include "Server.hpp"
#include "Exceptions.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>

Server::Server(int port, const string& password) : socketFd(-1), port(port), password(password) {}

void Server::start() {
	int socketFd;
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
}
