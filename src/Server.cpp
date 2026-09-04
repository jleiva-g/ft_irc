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
#include <iomanip>

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

			//TODO: Process the command and generate a response to queue for the client.
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

void Server::setPassAccepted(Client& client) { client.setPassAccepted(true); }

void Server::setNickname(Client& client, const string& nickname) {
	if (!client.getNickname().empty()) {
		stringstream ss;
		ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host NICK :" << nickname;
		
		nicknames.erase(client.getNickname());
		queueMessage(findPollfd(client.getFd()), ss.str());
		// TODO: Notify other clients in the same channels about the nickname change.
	}
	client.setNickname(nickname);
	nicknames[nickname] = &client;
}

void Server::setRegistered(Client& client) {
	client.setRegistered(true);
	stringstream ss;

	ss << ":Welcome to the Internet Relay Network " << client.getNickname() << "!" << client.getUsername() << "@host";
	sendCodeToClient(client, 001, ss.str());

	ss.str("");
	ss << ":Your host is ft_irc.server, running version 1.0";
	sendCodeToClient(client, 002, ss.str());

	ss.str("");
	ss << ":This server was created on " << __DATE__;
	sendCodeToClient(client, 003, ss.str());

	ss.str("");
	ss << ":Server info: ft_irc.server 1.0";
	sendCodeToClient(client, 004, ss.str());

	ss.str("");
	ss << ":There is no message of the day";
	sendCodeToClient(client, 422, ss.str());
}

bool Server::isNicknameInUse(const string& nickname) const { return nicknames.find(nickname) != nicknames.end(); }

void Server::leaveAllChannels(Client& client) {
	for (channel_iterator it = channels.begin(); it != channels.end(); it++)
		if (it->second->isMember(&client))
			partChannel(client, it->first, "Client disconnected");
}

void Server::joinChannel(Client& client, const string& channelName, const string& key) {
	//TODO: Manage channel keys and invite-only channels.
	Channel* channel;
	channel_iterator it = channels.find(channelName);
	if (it == channels.end()) {
		channel = new Channel(channelName);
		channels[channelName] = channel;
	}
	else
		channel = it->second;

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host JOIN :" << channelName;
	notifyChannelChange(client, channelName, ss.str());
	queueMessage(findPollfd(client.getFd()), ss.str());
	
	//channel->addMember(client);
}

void Server::partChannel(Client& client, const string& channelName, const string& reason) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host PART " << channelName;
	if (!reason.empty())
		ss << " :" << reason;
	notifyChannelChange(client, channelName, ss.str());
	queueMessage(findPollfd(client.getFd()), ss.str());
	//channel->removeMember(client);
	if (channel->getMembers().empty()) {
		delete channel;
		channels.erase(it->first);
	}
}

void Server::sendCodeToClient(Client& client, int code, const string& msg) {
	stringstream ss;
	ss << ":ft_irc.server " << std::setfill('0') << std::setw(3) << code << " " << client.getNickname() << " " << msg;
	queueMessage(findPollfd(client.getFd()), ss.str());
}

void Server::notifyChannelChange(Client& client, const string& channelName, const string& msg) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	const set<Client*>& members = channel->getMembers();
	for (set<Client*>::const_iterator mit = members.begin(); mit != members.end(); mit++) {
		Client* member = *mit;
		if (member != &client) {
			queueMessage(findPollfd(member->getFd()), msg);
		}
	}
}

void Server::broadcastToChannel(const string& channelName, const string& msg, Client* excludeClient) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	const set<Client*>& members = channel->getMembers();
	for (set<Client*>::const_iterator mit = members.begin(); mit != members.end(); mit++) {
		Client* member = *mit;
		if (member != excludeClient)
			queueMessage(findPollfd(member->getFd()), msg);
	}
}

void Server::sendMsgToClient(Client& client, const string& target, const string& msg) {
	nickname_iterator it = nicknames.find(target);
	Client* targetClient = it->second;

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host PRIVMSG " << target << " :" << msg;
	queueMessage(findPollfd(targetClient->getFd()), ss.str());
}

void Server::sendMsgToChannel(Client& client, const string& channelName, const string& msg) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	const set<Client*>& members = channel->getMembers();
	for (set<Client*>::const_iterator mit = members.begin(); mit != members.end(); mit++) {
		Client* member = *mit;
		if (member != &client) {
			stringstream ss;
			ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host PRIVMSG " << channelName << " :" << msg;
			queueMessage(findPollfd(member->getFd()), ss.str());
		}
	}
}

void Server::kickClient(Client& client, const string& channelName, const string& targetNickname, const string& reason) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host KICK " << channelName << " " << targetNickname << " :" << (reason.empty() ? "No reason provided" : reason);
	notifyChannelChange(client, channelName, ss.str());
	//channel->removeMember(*targetClient);
}

void Server::kickClient(Client& client, const string& channelName, const vector<string>& targetClients, const string& reason) {
	for (size_t i = 0; i < targetClients.size(); i++)
		kickClient(client, channelName, targetClients[i], reason);
}

void Server::inviteClient(Client& client, const string& targetNickname, const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host INVITE " << targetNickname << " :" << channelName;
	queueMessage(findPollfd(targetClient->getFd()), ss.str());

	ss.str("");
	ss << targetNickname << " " << channelName;
	sendCodeToClient(client, 341, ss.str());

	//channel->inviteClient(*targetClient);
}

void Server::sendTopic(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	stringstream ss;
	int code = channel->getTopic().empty() ? 331 : 332;
	if (channel->getTopic().empty())
		ss << channelName << " :No topic is set for this channel";
	else
		ss << channelName << " :" << channel->getTopic();
	sendCodeToClient(client, code, ss.str());
}

void Server::setTopic(Client& client, const string& channelName, const string& topic) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	//channel->setTopic(topic);

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host TOPIC " << channelName << " :" << topic;
	queueMessage(findPollfd(client.getFd()), ss.str());
	notifyChannelChange(client, channelName, ss.str());
}

void Server::sendChannelModes(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	string modes = "+";
	if (channel->isInviteOnly())
		modes += "i";
	if (channel->isTopicOpOnly())
		modes += "t";
	if (channel->hasKey())
		modes += "k";
	if (channel->hasUserLimit())
		modes += "l";
	
	stringstream ss;
	ss << channelName << " " << modes;
	if (channel->hasKey())
		ss << " " << channel->getKey();
	if (channel->hasUserLimit())
		ss << " " << channel->getUserLimit();
	sendCodeToClient(client, 324, ss.str());
}

// TODO Add this at the end of the modifications of the channel modes, send the list of operators in the channel.
void Server::sendChannelModesToAll(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	string modes = "+";
	if (channel->isInviteOnly())
		modes += "i";
	if (channel->isTopicOpOnly())
		modes += "t";
	if (channel->hasKey())
		modes += "k";
	if (channel->hasUserLimit())
		modes += "l";

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host MODE " << channelName << " " << modes;
	if (channel->hasKey())
		ss << " " << channel->getKey();
	if (channel->hasUserLimit())
		ss << " " << channel->getUserLimit();

	notifyChannelChange(client, channelName, ss.str());
	queueMessage(findPollfd(client.getFd()), ss.str());
}

bool Server::canModifyChannel(Client& client, const string& channelName) const {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	return channel->isOperator(&client);
}

void Server::setInviteOnly(Client& client, const string& channelName, bool inviteOnly) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	//channel->setInviteOnly(inviteOnly);
}

void Server::setTopicRestricted(Client& client, const string& channelName, bool topicOpOnly) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	//channel->setTopicOpOnly(topicOpOnly);
}

void Server::setChannelKey(Client& client, const string& channelName, const string& key) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	//channel->setKey(key);
}

void Server::removeChannelKey(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	//channel->removeKey();
}

void Server::setChannelOperator(Client& client, const string& channelName, const string& targetNickname) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	//channel->addOperator(targetClient);
}

void Server::removeChannelOperator(Client& client, const string& channelName, const string& targetNickname) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	channel->removeOperator(targetClient);
}

void Server::setUserLimit(Client& client, const string& channelName, size_t userLimit) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	//channel->setUserLimit(userLimit);
}

void Server::removeUserLimit(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	//channel->removeUserLimit();
}

bool Server::isClientInChannel(Client& client, const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->isMember(&client);
}

bool Server::isClientInvitedToChannel(Client& client, const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	//return channel->isInvited(&client);
	return false; // Placeholder until invite tracking is implemented
}

bool Server::isChannelInviteOnly(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->isInviteOnly();
}

bool Server::isChannelTopicRestricted(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->isTopicOpOnly();
}

bool Server::isChannelKeyProtected(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	//return channel->isKeyProtected();
	return false; // Placeholder until key protection is implemented
}

bool Server::isChannelFull(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	//return channel->isFull();
	return false; // Placeholder until user limit tracking is implemented
}

bool Server::isChannelOperator(const string& channelName, const string& nickname) const {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(nickname);

	Client* client = nit->second;
	return channel->isOperator(client);
}

bool Server::channelExists(const string& channelName) const {
	return channels.find(channelName) != channels.end();
}

bool Server::isNicknameRegistered(const string& nickname) const {
	return nicknames.find(nickname) != nicknames.end();
}

bool Server::isChannelPass(const string& channelName, const string& pass) const {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end()) {
		return false;
	}
	Channel* channel = it->second;
	//return channel->isKeyProtected() && channel->getKey() == pass;
	return false; // Placeholder until key protection is implemented
}

bool Server::passMatch(const string& pass) const {
	return pass == password;
}
