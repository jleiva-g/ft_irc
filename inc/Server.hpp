#pragma once

/**
 * @file Server.hpp
 * @brief Declaration of the IRC server class.
 * @details Defines the `Server` class, which manages the listening socket,
 *  connected clients, and channels. It provides methods for starting the server,
 *  accepting new clients, processing incoming and outgoing data, and broadcasting
 *  messages to channels.
 * 
 * @date 2026-07-19
 * @author Jesus Leiva Guerrero
 * @author Emilio Garcia Burgos
 * @author Lilith Estévez Boeta
 */

#include <string>
#include <poll.h>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

class	Client;
class	Channel;
class	Command;

/**
 * @class Server
 * @brief Manages the IRC server and its client connections.
 *
 * @details Owns the listening socket, tracks connected clients and channels,
 *  and dispatches network events and client commands.
 */
class	Server {
	private:
		typedef map<int, Client*>::const_iterator		client_iterator;	///< Iterator for traversing the client map.
		typedef map<string, Client*>::const_iterator	nickname_iterator;	///< Iterator for traversing the nickname map.
		typedef map<string, Channel*>::const_iterator	channel_iterator;	///< Iterator for traversing the channel map.

		static const string		connectionAcceptMsg;	///< Welcome message queued for a newly accepted client.

		int						socketFd;				///< Listening socket file descriptor.
		int						port;					///< Port on which the server listens for connections.
		string					password;				///< Password required by the server.
		vector<pollfd>			pollFds;				///< File descriptors monitored with `poll()`.
		map<int, Client*>		clients;				///< Connected clients keyed by file descriptor.
		map<string, Client*>	nicknames;				///< Registered nicknames keyed by nickname string.
		map<string, Channel*>	channels;				///< Existing channels keyed by name.

		void			mainLoop();
		void			proccessPollfd(int i);
		void			proccessIn(int fd);
		void			proccessOut(pollfd& poll);
		void			removeClient(int fd);
		void			queueMessage(pollfd& poll, const string& msg);

		pollfd&			findPollfd(int fd);
	public:
		Server(int port, const string& password);
		~Server();

		void	start();
		void	changeNickname(const string& nickname, Client* client);
		void	sendReplyToClient(const string& nickname, int code, const string& msg);
		void	sendMessageToClient(const string& sender, const string& recipient, const string& msg);
		void	sendMessageToChannel(const string& sender, const string& channelName, const string& msg);
};