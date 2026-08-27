/**
 * @file Server.cpp
 * @brief Implements the `Server` class for managing an IRC server.
 * @details Implements the `Server` class, which owns the listening socket,
 *  tracks connected clients and channels, and dispatches network events and
 *  client commands. The server uses `poll()` to monitor multiple sockets for
 *  incoming and outgoing data, and handles new connections, client input,
 *  and output in a non-blocking manner.
 * 
 * @date 2026-07-19
 * @author Jesus Leiva Guerrero
 * @author Emilio Garcia Burgos
 * @author Lilith Estévez Boeta
 */

#include "Server.hpp"
#include "Exceptions.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream>

using std::stringstream;

const string Server::connectionAcceptMsg = ":Welcome to the Internet Relay Network pepito!usuario@host";

/**
 * @brief Constructs a `Server` instance with the specified port and password.
 * @details Initializes the server's listening socket descriptor to -1, sets the
 *  listening port and password, and prepares the internal data structures for
 *  managing clients and channels.
 * 
 * @param[in] port Port number on which the server will listen for connections.
 * @param[in] password Password required for clients to register with the server.
 */
Server::Server(int port, const string& password) : socketFd(-1), port(port), password(password) {}
/**
 * @brief Destroys the `Server` instance and cleans up resources.
 * @details Closes the listening socket and deletes all `Client` and `Channel`
 *  objects owned by the server. The destructor ensures that all dynamically
 *  allocated resources are properly released to prevent memory leaks.
 */
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
 * 
 * @warning This method blocks indefinitely and does not return under normal operation.
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
	if (i < 0 || static_cast<size_t>(i) >= pollFds.size())
		return;

	int fd = pollFds[i].fd;
	short revents = pollFds[i].revents;

	if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
		if (fd != socketFd)
			removeClient(fd);
		return;
	}

	// Readable and writable events may be reported together for the same socket.
	if (revents & POLLIN)
		proccessIn(fd);

	if (fd != socketFd && clients.find(fd) == clients.end())
		return;

	if (revents & POLLOUT) {
		proccessOut(pollFds[i]);
	}
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
		if (acceptRes < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				return;
			std::cerr << "accept() failed: " << std::strerror(errno) << std::endl;
			return;
		}

		// Keep client operations from blocking the server's event loop.
		int flags = fcntl(acceptRes, F_GETFL, 0);
		if (flags == -1 || fcntl(acceptRes, F_SETFL, flags | O_NONBLOCK) == -1) {
			close(acceptRes);
			return;
		}

		// Monitor the new client for incoming data and pending outgoing data.
		pollfd newUserfd;
		newUserfd.fd = acceptRes;
		newUserfd.events = POLLIN;
		newUserfd.revents = 0;

		// Queue the initial welcome response; it will be sent on POLLOUT.
		clients[acceptRes] = new Client();
		clients[acceptRes]->setFd(acceptRes);
		sendReplyToClient("", 001, "Welcome to the Internet Relay Network");

		pollFds.push_back(newUserfd);
		std::cout << "Accepted new client on fd " << acceptRes << std::endl;
	}
	else {
		client_iterator it = clients.find(fd);
		if (it == clients.end())
			return;
		Client* client = it->second;

		// Append received bytes so incomplete commands can be completed later.
		ssize_t bytes = recv(fd, buff, sizeof(buff), 0);
		if (bytes == 0) {
			removeClient(fd);
			return;
		}
		if (bytes < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				return;
			removeClient(fd);
			return;
		}

		string response(buff, static_cast<size_t>(bytes));
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
	client_iterator it = clients.find(poll.fd);
	if (it == clients.end())
		return;
	Client* client = it->second;
	string& output = client->getSendBuffer();
	if (output.empty()) {
		poll.events &= ~POLLOUT;
		return;
	}

	ssize_t sent = send(poll.fd, output.c_str(), output.length(), 0);

	if (sent > 0)
		output.erase(0, sent);
	else if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		return;
	else {
		removeClient(poll.fd);
		return;
	}

	if (output.empty())
		poll.events &= ~POLLOUT;
	
	if (sent > 0)
		std::cout << "Sent " << sent << " bytes to client on fd " << poll.fd << std::endl;
	else
		std::cout << "Failed to send data to client on fd " << poll.fd << std::endl;
}

/**
 * @brief Removes a client from the server.
 * @details Deletes the `Client` object, removes the client from the `clients`
 *  map, removes the client's descriptor from `pollFds`, and closes the socket.
 * 
 * @param[in] fd File descriptor of the client to remove.
 */
void Server::removeClient(int fd) {
	client_iterator it = clients.find(fd);
	if (it != clients.end()) {
		if (nicknames.find(it->second->getNickname()) != nicknames.end())
			nicknames.erase(it->second->getNickname());
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

	std::cout << "Closed connection with client on fd " << fd << std::endl;
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

pollfd& Server::findPollfd(int fd) {
	for (size_t i = 0; i < pollFds.size(); i++) {
		if (pollFds[i].fd == fd)
			return pollFds[i];
	}
	stringstream fdString;
	fdString << fd;
	throw std::runtime_error("Pollfd not found for fd " + fdString.str());
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
 * @throw AlredyLinkedFileDescriptorException If the socket is already bound.
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

/**
 * @brief Checks if a provided password matches the server's password.
 * @details Compares the provided password string with the server's configured
 *  password and returns true if they match, false otherwise.
 * 
 * @param[in] pass Password string to check against the server's password.
 * @return true if the passwords match, false otherwise.
 */
bool Server::checkPassword(const string& pass) const { return pass == password; }

/**
 * @brief Changes a client's nickname.
 * @details Updates the client's nickname and the `nicknames` map. If the new
 *  nickname is already in use, sends an error reply to the client.
 * 
 * @param[in] nickname New nickname to assign to the client.
 * @param[in] client Pointer to the `Client` object whose nickname is being changed.
 * @throw std::runtime_error If the client pointer is null.
 */
void Server::changeNickname(const string& nickname, Client* client) {
	if (!client)
		throw std::runtime_error("Client pointer is null");

	if (nicknames.find(nickname) != nicknames.end()) {
		sendReplyToClient(client->getNickname(), 433, nickname + " :Nickname is already in use");
		return;
	}

	nicknames[nickname] = client;
	if (!client->getNickname().empty())
		nicknames.erase(client->getNickname());
	client->setNickname(nickname);
}

/**
 * @brief Sends a reply message to a client.
 * @details Constructs a reply message with the specified code and text, and
 *  queues it for sending to the client identified by the nickname. If the
 *  nickname is not registered, the function does nothing.
 * 
 * @param[in] nickname Nickname of the client to send the reply to.
 * @param[in] code Numeric reply code to include in the message.
 * @param[in] msg Text of the reply message.
 * @throw std::runtime_error If the client is not found in the `nicknames` map.
 */
void Server::sendReplyToClient(const string& nickname, int code, const string& msg) {
	nickname_iterator it = nicknames.find(nickname);
	if (it == nicknames.end())
		throw std::runtime_error("Client not found");

	stringstream ss;
	ss << ":irc.miservidor.com " << code << " " << nickname << " " << msg;
	queueMessage(findPollfd(it->second->getFd()), ss.str());
}

/**
 * @brief Sends a private message to a client.
 * @details Constructs a private message with the specified sender, recipient, and content, and
 *  queues it for sending to the recipient. If the recipient is not registered, the function
 *  throws a runtime error.
 *
 * @param[in] sender Nickname of the client sending the message.
 * @param[in] recipient Nickname of the client to send the message to.
 * @param[in] msg Text of the private message.
 * @throw std::runtime_error If the recipient is not found in the `nicknames` map.
 */
void Server::sendMessageToClient(const string& sender, const string& recipient, const string& msg) {
	nickname_iterator it = nicknames.find(recipient);
	if (it == nicknames.end())
		throw std::runtime_error("Client not found");

	stringstream ss;
	ss << ":" << sender << " PRIVMSG " << recipient << " :" << msg;
	queueMessage(findPollfd(it->second->getFd()), ss.str());
}

/**
 * @brief Sends a message to all members of a channel.
 * @details Constructs a message with the specified sender, channel name, and content, and
 *  queues it for sending to all members of the channel except the sender. If the channel
 *  does not exist, the function throws a runtime error.
 * 
 * @param[in] sender Nickname of the client sending the message.
 * @param[in] channelName Name of the channel to send the message to.
 * @param[in] msg Text of the message to send to the channel.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::sendMessageToChannel(const string& sender, const string& channelName, const string& msg) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	Channel* channel = it->second;
	const std::set<Client*>& members = channel->getMembers();

	for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
		Client* member = *it;
		if (member->getNickname() != sender) {
			stringstream ss;
			ss << ":" << sender << " PRIVMSG " << channelName << " :" << msg;
			queueMessage(findPollfd(member->getFd()), ss.str());
		}
	}
}

/**
 * @brief Sends a raw message to a client.
 * @details Queues the specified raw message for sending to the client. If the
 *  client pointer is null, the function throws a runtime error.
 * 
 * @param[in] client Pointer to the `Client` object to send the raw message to.
 * @param[in] rawLine Raw message to send to the client.
 * @throw std::runtime_error If the client pointer is null.
 */
void Server::sendRaw(Client* client, const string& rawLine) {
	if (!client)
		throw std::runtime_error("Client pointer is null");

	queueMessage(findPollfd(client->getFd()), rawLine);
}


/**
 * @brief Broadcasts a message to all members of a channel, excluding a specific client.
 * @details Constructs a message with the specified raw line and channel name, and
 *  queues it for sending to all members of the channel except the excluded client.
 * 
 * @param[in] rawLine Raw message to broadcast to the channel.
 * @param[in] channelName Name of the channel to broadcast the message to.
 * @param[in] exclude Pointer to the `Client` object to exclude from receiving the message. If null, no client is excluded.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::broadcastToChannel(const string& rawLine, const string& channelName, Client* exclude) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	Channel* channel = it->second;
	const std::set<Client*>& members = channel->getMembers();

	for (std::set<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
		Client* member = *it;
		if (member != exclude) {
			queueMessage(findPollfd(member->getFd()), rawLine);
		}
	}
}

/**
 * @brief Adds a new channel to the server.
 * @details Creates a new `Channel` object with the specified name and adds it
 *  to the `channels` map. If a channel with the same name already exists, the
 *  function throws a runtime error.
 * 
 * @param[in] channelName Name of the new channel to add.
 * @throw std::runtime_error If a channel with the same name already exists.
 */
void Server::addChannel(const string& channelName) {
	if (channels.find(channelName) != channels.end())
		throw std::runtime_error("Channel already exists");

	Channel* channel = new Channel(channelName);
	channels[channelName] = channel;
}

/**
 * @brief Removes a channel from the server.
 * @details Deletes the `Channel` object and removes it from the `channels`
 *  map. If the channel does not exist, the function throws a runtime error.
 * 
 * @param[in] channelName Name of the channel to remove.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::removeChannel(const string& channelName) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	delete it->second;
	channels.erase(channelName);
}

/**
 * @brief Adds a client to a channel.
 * @details Adds the specified `Client` object to the member list of the
 *  `Channel` identified by the channel name. If the channel does not exist,
 *  the function throws a runtime error.
 *
 * @param[in] channelName Name of the channel to add the client to.
 * @param[in] client Pointer to the `Client` object to add to the channel.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::addClientToChannel(const string& channelName, Client* client) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	it->second->addMember(client);
}

/**
 * @brief Removes a client from a channel.
 * @details Removes the specified `Client` object from the member list of the
 *  `Channel` identified by the channel name. If the channel does not exist,
 *  the function throws a runtime error.
 *
 * @param[in] channelName Name of the channel to remove the client from.
 * @param[in] client Pointer to the `Client` object to remove from the channel.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::removeClientFromChannel(const string& channelName, Client* client) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	it->second->removeMember(client);
}

/**
 * @brief Adds an operator to a channel.
 * @details Adds the specified `Client` object to the operator list of the
 *  `Channel` identified by the channel name. If the channel does not exist,
 *  the function throws a runtime error.
 *
 * @param[in] channelName Name of the channel to add the operator to.
 * @param[in] client Pointer to the `Client` object to add as an operator.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::addOperatorToChannel(const string& channelName, Client* client) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	it->second->addOperator(client);
}

/**
 * @brief Removes an operator from a channel.
 * @details Removes the specified `Client` object from the operator list of the
 *  `Channel` identified by the channel name. If the channel does not exist,
 *  the function throws a runtime error.
 *
 * @param[in] channelName Name of the channel to remove the operator from.
 * @param[in] client Pointer to the `Client` object to remove from the channel.
 * @throw std::runtime_error If the channel is not found in the `channels` map.
 */
void Server::removeOperatorFromChannel(const string& channelName, Client* client) {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end())
		throw std::runtime_error("Channel not found");

	it->second->removeOperator(client);
}

Client* Server::getClient(int fd) const {
	client_iterator it = clients.find(fd);
	if (it != clients.end())
		return it->second;
	return NULL;
}

Client* Server::getClientByNickname(const string& nickname) const {
	nickname_iterator it = nicknames.find(nickname);
	if (it != nicknames.end())
		return it->second;
	return NULL;
}

bool Server::isNicknameInUse(const string& nickname) const { return nicknames.find(nickname) != nicknames.end(); }

Channel* Server::getChannel(const string& channelName) const {
	channel_iterator it = channels.find(channelName);
	if (it != channels.end())
		return it->second;
	return NULL;
}

bool Server::channelExists(const string& channelName) const { return channels.find(channelName) != channels.end(); }
