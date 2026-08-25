#include "Server.hpp"
#include "Exceptions.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sstream>
#include <iostream>

using std::stringstream;

const string Server::connectionAcceptMsg = ":irc.miservidor.com 001 pepito :Welcome to the Internet Relay Network pepito!usuario@host";

Server::Server(int port, const string& password) : socketFd(-1), port(port), password(password) {}
Server::~Server() {
	// The server owns the client and channel objects stored in these maps.
	for (client_iterator it = clients.begin(); it != clients.end(); it++)
		delete it->second;
	for (channel_iterator it = channels.begin(); it != channels.end(); it++)
		delete it->second;
}

/**
 * @brief Runs the server's main event loop.
 * @details Waits for activity on the listening socket and connected clients,
 *  then dispatches the corresponding input and output handlers.
 */
void Server::mainLoop() {
	while (true) {
		// Wait indefinitely until at least one monitored descriptor reports an event.
		int pollRes = poll(pollFds.data(), pollFds.size(), -1);
		if (pollRes < 0) {
			// Poll errors must be handled before processing the event list.
		}

		// Dispatch the events reported by poll() to the appropriate handlers.
		for (size_t i = 0; i < pollFds.size(); i++)
			proccessPollfd(i);
	}
}

/**
 * @brief Processes the events reported for a monitored file descriptor.
 * @details Dispatches readable and writable events to their respective
 *  handlers and reserves the remaining branches for connection and socket
 *  error handling.
 *
 * @param[in] i Index of the monitored file descriptor in `pollFds`.
 */
void Server::proccessPollfd(int i) {
	pollfd& socket = pollFds[i];
	int fd = socket.fd;

	// Readable and writable events may be reported together for the same socket.
	if (socket.revents & POLLIN)
		proccessIn(fd);
	if (socket.revents & POLLOUT)
		proccessOut(socket);
	// Reserved for closing the connection and removing the associated client.
	if (socket.revents & POLLHUP || socket.revents & POLLERR || socket.revents & POLLNVAL)
		removeClient(fd);
	// Reserved for handling urgent data reported by the socket.
	/*if (socket.revents & POLLPRI) {

	}*/
}

/**
 * @brief Processes pending input on a monitored socket.
 * @details Accepts a new connection when the listening socket is readable;
 *  otherwise, receives data from the corresponding client and appends it to
 *  the client's receive buffer.
 *
 * @param[in] fd File descriptor of the socket to process.
 */
void Server::proccessIn(int fd) {
	char buff[1024];
	if (fd == socketFd) {
		// A readable listening socket indicates that a client is waiting to connect.
		int acceptRes = accept(socketFd, NULL, NULL);

		// Keep client operations from blocking the server's event loop.
		int flags = fcntl(acceptRes, F_GETFL, 0);
		fcntl(acceptRes, F_SETFL, flags | O_NONBLOCK);

		// Monitor the new client for incoming data and pending outgoing data.
		pollfd newUserfd;
		newUserfd.fd = acceptRes;
		newUserfd.events = POLLIN;
		newUserfd.revents = 0;

		// Queue the initial welcome response; it will be sent on POLLOUT.
		clients[acceptRes] = new Client();
		queueMessage(newUserfd, connectionAcceptMsg);

		pollFds.push_back(newUserfd);
		std::cout << "Accepted new client on fd " << acceptRes << std::endl;
	}
	else {
		Client* client = clients[fd];

		// Append received bytes so incomplete commands can be completed later.
		ssize_t bytes = recv(fd, buff, sizeof(buff), 0);
		string response(buff, bytes);
		client->appendRecvData(response);

		while(client->getOneCommandFromBuffer(response)) {
			std::cout << "Received command from client on fd " << fd << ": " << response << std::endl;
		}
	}
}

/**
 * @brief Sends pending output to a client.
 * @details Sends as much of the client's output buffer as the socket accepts,
 *  removes the transmitted bytes, and disables `POLLOUT` when the buffer is empty.
 *
 * @param[in,out] poll Descriptor associated with the client whose requested
 *  events are updated.
 */
void Server::proccessOut(pollfd& poll) {
	Client* client = clients[poll.fd];
	// send() may transmit only part of the queued response.
	ssize_t sent = send(poll.fd, client->getOutputBuffer().c_str(), client->getOutputBuffer().length(), 0);

	if (sent > 0)
		client->getOutputBuffer().erase(0, sent);

	// Stop requesting writable events until more data is queued.
	if (client->getOutputBuffer().empty())
		poll.events &= ~POLLOUT;
	
	if (sent > 0)
		std::cout << "Sent " << sent << " bytes to client on fd " << poll.fd << std::endl;
	else
		std::cout << "Failed to send data to client on fd " << poll.fd << std::endl;
}

void Server::removeClient(int fd) {
	client_iterator it = clients.find(fd);
	if (it != clients.end()) {
		delete it->second;
		clients.erase(fd);
	}

	for (size_t i = 0; i < pollFds.size(); i++) {
		if (pollFds[i].fd == fd) {
			pollFds.erase(pollFds.begin() + i);
			break;
		}
	}

	close(fd);
} 

/**
 * @brief Queues a message for a client.
 * @details Adds the supplied message to the output buffer of the client
 *  associated with the descriptor and retains only the descriptor's
 *  `POLLOUT` event flag.
 *
 * @param[in,out] poll Descriptor associated with the client whose requested
 *  events are updated.
 * @param[in] msg Message to add to the client's output buffer.
 */
void Server::queueMessage(pollfd& poll, const string& msg) {
	clients[poll.fd]->queueOneCommandToBuffer(msg);
	poll.events |= POLLOUT;
}

/**
 * @brief Initializes the listening socket and starts the event loop.
 * @details Creates an IPv4 stream socket, binds it to the configured port,
 *  registers it with `poll()`, and listens for incoming connections.
 *
 * @throw UnknownException If an unknown socket, bind, or listen error occurs.
 * @throw NotEnoughPermissionsException If the process lacks the required permissions.
 * @throw NotEnoughMemoryException If the system cannot allocate the required resources.
 * @throw NotEnoughFileDescriptorsException If no file descriptors are available.
 * @throw UnsupportedIPProtocolException If IPv4 sockets are not supported.
 * @throw PortInUseException If the configured port is already in use.
 * @throw InvalidFileDescriptorException If a socket file descriptor is invalid.
 * @throw AlreadyLinkedFileDescriptorException If the socket is already bound.
 * @throw FileDescriptorIsNotSocketException If the file descriptor is not a socket.
 * @throw SocketNotSupportListenException If the socket does not support listening.
 */
void Server::start() {
	// Create an IPv4 TCP listening socket.
	if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		manageErrorsFromSocket(errno, __FILE__, __LINE__ - 1);

	sockaddr_in addr;
	// Bind on all local interfaces and the configured port.
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

	// The listening descriptor only needs to report incoming connections.
	pollfd listenfd;
	listenfd.fd = socketFd;
	listenfd.events = POLLIN;
	listenfd.revents = 0;

	pollFds.push_back(listenfd);

	mainLoop();
}
