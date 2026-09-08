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
#include "Command.hpp"
#include "Numeric.hpp"
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

static string escapeForLog(const string& value) {
	static const char hex[] = "0123456789ABCDEF";
	string escaped;

	for (size_t i = 0; i < value.size(); ++i) {
		unsigned char character = static_cast<unsigned char>(value[i]);
		if (character == '\\')
			escaped += "\\\\";
		else if (character == '\r')
			escaped += "\\r";
		else if (character == '\n')
			escaped += "\\n";
		else if (character == '\t')
			escaped += "\\t";
		else if (character < 32 || character == 127) {
			escaped += "\\x";
			escaped += hex[character >> 4];
			escaped += hex[character & 0x0F];
		}
		else
			escaped += static_cast<char>(character);
	}
	return escaped;
}

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
		int pollRes = poll(pollFds.data(), pollFds.size(), -1);
		if (pollRes < 0) {
			// Poll errors must be handled before processing the event list.
		}

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
		int acceptRes = accept(socketFd, NULL, NULL);
		if (acceptRes < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				return;
			std::cerr << "accept() failed: " << std::strerror(errno) << std::endl;
			return;
		}

		int flags = fcntl(acceptRes, F_GETFL, 0);
		if (flags == -1 || fcntl(acceptRes, F_SETFL, flags | O_NONBLOCK) == -1) {
			close(acceptRes);
			return;
		}

		pollfd newUserfd;
		newUserfd.fd = acceptRes;
		newUserfd.events = POLLIN;
		newUserfd.revents = 0;

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

		//std::cout << "Received " << bytes << " bytes from client on fd " << fd << std::endl;
		//std::cout << "Data: " << escapeForLog(response) << std::endl;

		while(client->getOneCommandFromBuffer(response)) {
			std::cout << "Received command from client on fd " << fd << ": " << escapeForLog(response) << std::endl;
			Command::handleCommand(*client, *this, response);
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
 * @brief Retrieves a client by their file descriptor.
 * @details Searches the server's client map for a client with the specified
 *  file descriptor and returns a pointer to the client if found, or NULL
 *  if not found.
 * 
 * @param[in] fd File descriptor of the client to retrieve.
 * @return Pointer to the client if found, NULL otherwise.
 */
Client* Server::getClientByFd(int fd) const {
	client_iterator it = clients.find(fd);
	if (it != clients.end())
		return it->second;
	return NULL;
}

/**
 * @brief Retrieves a client by their nickname.
 * @details Searches the server's nickname map for a client with the specified
 *  nickname and returns a pointer to the client if found, or NULL if not found.
 * 
 * @param[in] nickname Nickname of the client to retrieve.
 * @return Pointer to the client if found, NULL otherwise.
 */
Client* Server::getClientByNickname(const string& nickname) const {
	nickname_iterator it = nicknames.find(nickname);
	if (it != nicknames.end())
		return it->second;
	return NULL;
}

/**
 * @brief Marks a client as having accepted the server password.
 * @details Sets the `passAccepted` flag of the specified client to true,
 *  indicating that the client has successfully authenticated with the server.
 * 
 * @param[in,out] client Client to mark as having accepted the password.
 */
void Server::setPassAccepted(Client& client) { client.setPassAccepted(true); }

/**
 * @brief Sets a client's nickname and notifies other clients of the change.
 * @details Updates the client's nickname, removes the old nickname from the
 *  server's nickname map, and sends a notification to all clients in the same
 *  channels about the nickname change.
 * 
 * @param[in,out] client Client whose nickname is being changed.
 * @param[in] nickname New nickname to assign to the client.
 */
void Server::setNickname(Client& client, const string& nickname) {
	if (!client.getNickname().empty()) {
		stringstream ss;
		ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host NICK :" << nickname;
		
		nicknames.erase(client.getNickname());
		queueMessage(findPollfd(client.getFd()), ss.str());
		set<Client*> notifiedClients;
		for (channel_iterator it = channels.begin(); it != channels.end(); it++) {
			Channel* channel = it->second;
			if (!channel->isMember(&client)) continue;
			const set<Client*>& members = channel->getMembers();
			for (set<Client*>::const_iterator memberIt = members.begin(); memberIt != members.end(); ++memberIt) {
				Client* member = *memberIt;
				if (member != &client && notifiedClients.insert(member).second)
					queueMessage(findPollfd(member->getFd()), ss.str());
			}
		}
	}
	client.setNickname(nickname);
	nicknames[nickname] = &client;
}

/**
 * @brief Sets a client's registered status.
 * @details Sets the `registered` flag of the specified client to true,
 *  indicating that the client has successfully registered with the server.
 * 
 * @param[in,out] client Client to mark as registered.
 */
void Server::setRegistered(Client& client) {
	client.setRegistered(true);

	sendCodeToClient(client, RPL_WELCOME, getNumericInfo(RPL_WELCOME).message + " " + client.getNickname() + "!" + client.getUsername() + "@host");
	sendCodeToClient(client, RPL_YOURHOST, getNumericInfo(RPL_YOURHOST).message + " ft_irc.server, running version 1.0");
	sendCodeToClient(client, RPL_CREATED, getNumericInfo(RPL_CREATED).message + " " + __DATE__);
	sendCodeToClient(client, RPL_MYINFO, getNumericInfo(RPL_MYINFO).message + " ft_irc.server 1.0");
	sendCodeToClient(client, ERR_NOMOTD, getNumericInfo(ERR_NOMOTD).message);
}

/**
 * @brief Checks if a nickname is already in use by a client.
 * @details Searches the server's nickname map for a client with the specified
 *  nickname and returns true if found, false otherwise.
 *
 * @param[in] nickname Nickname to check.
 * @return true if the nickname is in use, false otherwise.
 */
bool Server::isNicknameInUse(const string& nickname) const { return nicknames.find(nickname) != nicknames.end(); }

/**
 * @brief Removes a client from all channels.
 * @details Iterates through all channels and removes the specified client from each
 *  channel they are a member of.
 *
 * @param[in,out] client Client to remove from all channels.
 */
void Server::leaveAllChannels(Client& client) {
	for (channel_iterator it = channels.begin(); it != channels.end(); it++)
		if (it->second->isMember(&client))
			partChannel(client, it->first, "Client disconnected");
}

/**
 * @brief Joins a client to a channel.
 * @details Adds the specified client to the channel with the given name.
 *  If the channel does not exist, it is created.
 *
 * @param[in,out] client Client to join the channel.
 * @param[in] channelName Name of the channel to join.
 * @param[in] key Key for the channel, if required.
 */
void Server::joinChannel(Client& client, const string& channelName, const string& key) {
	Channel* channel;
	channel_iterator it = channels.find(channelName);
	if (it == channels.end()) {
		channel = new Channel(channelName);
		channels[channelName] = channel;
	}
	else
		channel = it->second;

	if (channel->hasModeInviteOnly() && !channel->isInvited(&client)) {
		sendCodeToClient(client, ERR_INVITEONLYCHAN, channelName + getNumericInfo(ERR_INVITEONLYCHAN).message);
		return;
	}
	if (channel->hasModeKey() && channel->getKey() != key) {
		sendCodeToClient(client, ERR_BADCHANNELKEY, channelName + getNumericInfo(ERR_BADCHANNELKEY).message);
		return;
	}
	if (channel->hasModeUserLimit() && channel->getMembers().size() >= channel->getUserLimit()) {
		sendCodeToClient(client, ERR_CHANNELISFULL, channelName + getNumericInfo(ERR_CHANNELISFULL).message);
		return;
	}

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host JOIN :" << channelName;
	notifyChannelChange(client, channelName, ss.str());
	queueMessage(findPollfd(client.getFd()), ss.str());
	
	channel->addMember(&client);
}

/**
 * @brief Parts a client from a channel.
 * @details Removes the specified client from the channel with the given name.
 *
 * @param[in,out] client Client to part from the channel.
 * @param[in] channelName Name of the channel to part from.
 * @param[in] reason Reason for parting the channel.
 */
void Server::partChannel(Client& client, const string& channelName, const string& reason) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host PART " << channelName;
	if (!reason.empty())
		ss << " :" << reason;
	notifyChannelChange(client, channelName, ss.str());
	queueMessage(findPollfd(client.getFd()), ss.str());
	channel->removeMember(&client);
	if (channel->getMembers().empty()) {
		delete channel;
		channels.erase(it->first);
	}
}

/**
 * @brief Sends a numeric message to a client.
 * @details Formats a numeric message with the specified code and message,
 *  and queues it for delivery to the client.
 *
 * @param[in,out] client Client to send the message to.
 * @param[in] code Numeric code for the message.
 * @param[in] msg Message to send.
 */
void Server::sendCodeToClient(Client& client, int code, const string& msg) {
	stringstream ss;
	ss << ":ft_irc.server " << std::setfill('0') << std::setw(3) << code << " " << client.getNickname() << " " << msg;
	queueMessage(findPollfd(client.getFd()), ss.str());
}

/**
 * @brief Notifies all members of a channel about a change.
 * @details Sends a message to all clients in the specified channel, excluding
 *  the client who initiated the change.
 * 
 * @param[in,out] client Client who initiated the change.
 * @param[in] channelName Name of the channel where the change occurred.
 * @param[in] msg Message to send to the channel members.
 */
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

/**
 * @brief Broadcasts a message to all members of a channel, excluding a specific client.
 * @details Sends a message to all clients in the specified channel, excluding the
 *  client specified by `excludeClient`.
 *
 * @param[in] channelName Name of the channel to broadcast the message to.
 * @param[in] msg Message to broadcast.
 * @param[in] excludeClient Client to exclude from receiving the message.
 */
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

/**
 * @brief Sends a private message to a client.
 * @details Formats a private message with the sender's nickname and username,
 *  and queues it for delivery to the target client.
 *
 * @param[in,out] client Client who is sending the message.
 * @param[in] target Nickname of the client to send the message to.
 * @param[in] msg Message to send.
 */
void Server::sendMsgToClient(Client& client, const string& target, const string& msg) {
	nickname_iterator it = nicknames.find(target);
	Client* targetClient = it->second;

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host PRIVMSG " << target << " :" << msg;
	queueMessage(findPollfd(targetClient->getFd()), ss.str());
}

/**
 * @brief Sends a message to all members of a channel, excluding the sender.
 * @details Formats a message with the sender's nickname and username, and
 *  queues it for delivery to all clients in the specified channel, excluding
 *  the sender.
 *
 * @param[in,out] client Client who is sending the message.
 * @param[in] channelName Name of the channel to send the message to.
 * @param[in] msg Message to send.
 */
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

/**
 * @brief Kicks a client from a channel.
 * @details Removes the specified client from the channel with the given name.
 *
 * @param[in,out] client Client who is kicking another client.
 * @param[in] channelName Name of the channel to kick the client from.
 * @param[in] targetNickname Nickname of the client to kick.
 * @param[in] reason Reason for kicking the client.
 */
void Server::kickClient(Client& client, const string& channelName, const string& targetNickname, const string& reason) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host KICK " << channelName << " " << targetNickname << " :" << (reason.empty() ? "No reason provided" : reason);
	notifyChannelChange(client, channelName, ss.str());
	channel->removeMember(targetClient);
}

/**
 * @brief Kicks multiple clients from a channel.
 * @details Removes the specified clients from the channel with the given name.
 *
 * @param[in,out] client Client who is kicking other clients.
 * @param[in] channelName Name of the channel to kick the clients from.
 * @param[in] targetClients Vector of nicknames of the clients to kick.
 * @param[in] reason Reason for kicking the clients.
 */
void Server::kickClient(Client& client, const string& channelName, const vector<string>& targetClients, const string& reason) {
	for (size_t i = 0; i < targetClients.size(); i++)
		kickClient(client, channelName, targetClients[i], reason);
}

/**
 * @brief Invites a client to a channel.
 * @details Sends an invitation to the specified client to join the channel.
 *
 * @param[in,out] client Client who is inviting another client.
 * @param[in] targetNickname Nickname of the client to invite.
 * @param[in] channelName Name of the channel to invite the client to.
 */
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

	channel->addInvited(targetClient);
}

/**
 * @brief Sends the topic of a channel to a client.
 * @details Retrieves the topic of the specified channel and sends it to the client.
 *
 * @param[in,out] client Client to send the topic to.
 * @param[in] channelName Name of the channel to get the topic from.
 */
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

/**
 * @brief Sets the topic of a channel.
 * @details Updates the topic of the specified channel and notifies all members.
 *
 * @param[in,out] client Client who is setting the topic.
 * @param[in] channelName Name of the channel to set the topic for.
 * @param[in] topic New topic for the channel.
 */
void Server::setTopic(Client& client, const string& channelName, const string& topic) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	channel->setTopic(topic);

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host TOPIC " << channelName << " :" << topic;
	queueMessage(findPollfd(client.getFd()), ss.str());
	notifyChannelChange(client, channelName, ss.str());
}

/**
 * @brief Sends the modes of a channel to a client.
 * @details Retrieves the modes of the specified channel and sends them to the client.
 *
 * @param[in,out] client Client to send the modes to.
 * @param[in] channelName Name of the channel to get the modes from.
 */
void Server::sendChannelModes(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	string modes = "+";
	if (channel->hasModeInviteOnly())
		modes += "i";
	if (channel->hasModeTopicOpOnly())
		modes += "t";
	if (channel->hasModeKey())
		modes += "k";
	if (channel->hasModeUserLimit())
		modes += "l";
	
	stringstream ss;
	ss << channelName << " " << modes;
	if (channel->hasModeKey())
		ss << " " << channel->getKey();
	if (channel->hasModeUserLimit())
		ss << " " << channel->getUserLimit();
	sendCodeToClient(client, 324, ss.str());
}

/**
 * @brief Sends the modes of a channel to all members.
 * @details Retrieves the modes of the specified channel and sends them to all members.
 *
 * @param[in,out] client Client who is sending the modes.
 * @param[in] channelName Name of the channel to get the modes from.
 */
void Server::sendChannelModesToAll(Client& client, const string& channelName) {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	string modes = "+";
	if (channel->hasModeInviteOnly())
		modes += "i";
	if (channel->hasModeTopicOpOnly())
		modes += "t";
	if (channel->hasModeKey())
		modes += "k";
	if (channel->hasModeUserLimit())
		modes += "l";

	stringstream ss;
	ss << ":" << client.getNickname() << "!" << client.getUsername() << "@host MODE " << channelName << " " << modes;
	if (channel->hasModeKey())
		ss << " " << channel->getKey();
	if (channel->hasModeUserLimit())
		ss << " " << channel->getUserLimit();

	notifyChannelChange(client, channelName, ss.str());
	queueMessage(findPollfd(client.getFd()), ss.str());
}

/**
 * @brief Checks if a client can modify a channel.
 * @details Verifies if the specified client is an operator of the channel.
 *
 * @param[in] client Client to check.
 * @param[in] channelName Name of the channel to check.
 * @return true if the client is an operator of the channel, false otherwise.
 */
bool Server::canModifyChannel(Client& client, const string& channelName) const {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	return channel->isOperator(&client);
}

/**
 * @brief Sets the invite-only mode of a channel.
 * @details Enables or disables the invite-only mode for the specified channel.
 *
 * @param[in,out] client Client who is setting the mode.
 * @param[in] channelName Name of the channel to set the mode for.
 * @param[in] inviteOnly true to enable invite-only mode, false to disable it.
 */
void Server::setInviteOnly(const string& channelName, bool inviteOnly) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	channel->setInviteOnly(inviteOnly);
}

/**
 * @brief Sets the topic restriction mode of a channel.
 * @details Enables or disables the topic restriction mode for the specified channel.
 *
 * @param[in,out] client Client who is setting the mode.
 * @param[in] channelName Name of the channel to set the mode for.
 * @param[in] topicOpOnly true to enable topic restriction, false to disable it.
 */
void Server::setTopicRestricted(const string& channelName, bool topicOpOnly) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	channel->setTopicOpOnly(topicOpOnly);
}

/**
 * @brief Sets the key for a channel.
 * @details Assigns a key to the specified channel and marks it as having a key.
 * 
 * @param[in,out] client Client who is setting the key.
 * @param[in] channelName Name of the channel to set the key for.
 * @param[in] key Key to assign to the channel.
 */
void Server::setChannelKey(const string& channelName, const string& key) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	channel->setKey(key);
	channel->setHasKey(true);
}

/**
 * @brief Removes the key from a channel.
 * @details Removes the key from the specified channel and marks it as not having a key.
 *
 * @param[in,out] client Client who is removing the key.
 * @param[in] channelName Name of the channel to remove the key from.
 */
void Server::removeChannelKey(const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	channel->setHasKey(false);
}

/**
 * @brief Sets a client as an operator of a channel.
 * @details Adds the specified client as an operator of the channel with the given name.
 * 
 * @param[in,out] client Client who is setting the operator.
 * @param[in] channelName Name of the channel to set the operator for.	
 * @param[in] targetNickname Nickname of the client to set as an operator.
 */
void Server::setChannelOperator(const string& channelName, const string& targetNickname) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	channel->addOperator(targetClient);
}

/**
 * @brief Removes a client as an operator of a channel.
 * @details Removes the specified client as an operator of the channel with the given name.
 *
 * @param[in,out] client Client who is removing the operator.
 * @param[in] channelName Name of the channel to remove the operator from.
 * @param[in] targetNickname Nickname of the client to remove as an operator.
 */
void Server::removeChannelOperator(const string& channelName, const string& targetNickname) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(targetNickname);

	Client* targetClient = nit->second;
	channel->removeOperator(targetClient);
}

/**
 * @brief Sets the user limit for a channel.
 * @details Sets the maximum number of users allowed in the specified channel.
 *
 * @param[in,out] client Client who is setting the user limit.
 * @param[in] channelName Name of the channel to set the user limit for.
 * @param[in] userLimit Maximum number of users allowed in the channel.
 */
void Server::setUserLimit(const string& channelName, size_t userLimit) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	channel->setUserLimit(userLimit);
}

/**
 * @brief Removes the user limit from a channel.
 * @details Removes the user limit from the specified channel.
 *
 * @param[in,out] client Client who is removing the user limit.
 * @param[in] channelName Name of the channel to remove the user limit from.
 */
void Server::removeUserLimit(const string& channelName) {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;

	channel->setUserLimit(-1);
}

/**
 * @brief Checks if a client is in a channel.
 * @details Determines if the specified client is a member of the channel with the given name.
 *
 * @param[in] client Client to check.
 * @param[in] channelName Name of the channel to check.
 * @return true if the client is in the channel, false otherwise.
 */
bool Server::isClientInChannel(Client& client, const string& channelName) const {
	if (!channelExists(channelName)) {
		return false;
	}
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->isMember(&client);
}

/**
 * @brief Checks if a client is invited to a channel.
 * @details Determines if the specified client is invited to the channel with the given name.
 *
 * @param[in] client Client to check.
 * @param[in] channelName Name of the channel to check.
 * @return true if the client is invited to the channel, false otherwise.
 */
bool Server::isClientInvitedToChannel(Client& client, const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->isInvited(&client);
}

/**
 * @brief Checks if a channel is invite-only.
 * @details Determines if the specified channel is invite-only.
 *
 * @param[in] channelName Name of the channel to check.
 * @return true if the channel is invite-only, false otherwise.
 */
bool Server::isChannelInviteOnly(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->hasModeInviteOnly();
}

/**
 * @brief Checks if a channel's topic is restricted.
 * @details Determines if the topic of the specified channel is restricted to operators only.
 *
 * @param[in] channelName Name of the channel to check.
 * @return true if the channel's topic is restricted, false otherwise.
 */
bool Server::isChannelTopicRestricted(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->hasModeTopicOpOnly();
}

/**
 * @brief Checks if a channel is key-protected.
 * @details Determines if the specified channel is protected by a password.
 *
 * @param[in] channelName Name of the channel to check.
 * @return true if the channel is key-protected, false otherwise.
 */
bool Server::isChannelKeyProtected(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->hasModeKey();
}

/**
 * @brief Checks if a channel is full.
 * @details Determines if the specified channel has reached its user limit.
 *
 * @param[in] channelName Name of the channel to check.
 * @return true if the channel is full, false otherwise.
 */
bool Server::isChannelFull(const string& channelName) const {
	channel_iterator it = channels.find(channelName);

	Channel* channel = it->second;
	return channel->isFull();
}

/**
 * @brief Checks if a client is an operator of a channel.
 * @details Determines if the specified client is an operator of the channel with the given name.
 *
 * @param[in] channelName Name of the channel to check.
 * @param[in] nickname Nickname of the client to check.
 * @return true if the client is an operator of the channel, false otherwise.
 */
bool Server::isChannelOperator(const string& channelName, const string& nickname) const {
	channel_iterator it = channels.find(channelName);
	Channel* channel = it->second;
	nickname_iterator nit = nicknames.find(nickname);

	Client* client = nit->second;
	return channel->isOperator(client);
}

/**
 * @brief Checks if a channel exists.
 * @details Determines if the specified channel exists.
 *
 * @param[in] channelName Name of the channel to check.
 * @return true if the channel exists, false otherwise.
 */
bool Server::channelExists(const string& channelName) const {
	return channels.find(channelName) != channels.end();
}

/**
 * @brief Checks if a nickname is registered.
 * @details Determines if the specified nickname is registered.
 *
 * @param[in] nickname Nickname to check.
 * @return true if the nickname is registered, false otherwise.
 */
bool Server::isNicknameRegistered(const string& nickname) const {
	return nicknames.find(nickname) != nicknames.end();
}

/**
 * @brief Checks if a channel's password is correct.
 * @details Determines if the specified password matches the password for the channel.
 *
 * @param[in] channelName Name of the channel to check.
 * @param[in] pass Password to check.
 * @return true if the password is correct, false otherwise.
 */
bool Server::isChannelPass(const string& channelName, const string& pass) const {
	channel_iterator it = channels.find(channelName);
	if (it == channels.end()) {
		return false;
	}
	Channel* channel = it->second;
	return channel->hasModeKey() && channel->getKey() == pass;
}

/**
 * @brief Checks if a password matches the server's password.
 * @details Determines if the specified password matches the server's password.
 *
 * @param[in] pass Password to check.
 * @return true if the password is correct, false otherwise.
 */
bool Server::passMatch(const string& pass) const { return pass == password; }

/**
 * @brief Checks if a client has enough channels.
 * @details Determines if the specified client has joined enough channels.
 *
 * @param[in] client Client to check.
 * @return true if the client has enough channels, false otherwise.
 */
bool Server::clientHasEnoughChannels(Client& client) const { return client.hasEnoughChannels(); }
