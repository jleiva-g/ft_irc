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

class	Client;
class	Channel;

/**
 * @class Server
 * @brief Manages the IRC server and its client connections.
 *
 * @details Owns the listening socket, tracks connected clients and channels,
 *  and dispatches network events and client commands.
 */
class	Server {
	private:
		typedef std::map<int, Client*>::const_iterator			client_iterator;	///< Iterator for traversing the client map.
		typedef std::map<std::string, Client*>::const_iterator	nickname_iterator;	///< Iterator for traversing the nickname map.
		typedef std::map<std::string, Channel*>::const_iterator	channel_iterator;	///< Iterator for traversing the channel map.

		static const std::string		connectionAcceptMsg;	///< Welcome message queued for a newly accepted client.

		int								socketFd;				///< Listening socket file descriptor.
		int								port;					///< Port on which the server listens for connections.
		std::string						password;				///< Password required by the server.
		std::vector<pollfd>				pollFds;				///< File descriptors monitored with `poll()`.
		std::map<int, Client*>			clients;				///< Connected clients keyed by file descriptor.
		std::map<std::string, Client*>	nicknames;				///< Registered nicknames keyed by nickname string.
		std::map<std::string, Channel*>	channels;			///< Existing channels keyed by name.

		void			mainLoop();
		void			proccessPollfd(int i);
		void			proccessIn(int fd);
		void			proccessOut(pollfd& poll);
		void			removeClient(int fd);
		void			queueMessage(pollfd& poll, const std::string& msg);

		pollfd&			findPollfd(int fd);
	public:
		Server(int port, const std::string& password);
		~Server();

		void	start();
		bool	checkPassword(const std::string& pass) const;
		void	changeNickname(const std::string& nickname, Client* client);

		/**
		 * @defgroup SendingMessages Sending messages to clients and channels
		 * @brief Methods for sending messages to clients and channels.
		 * @details These methods handle the formatting and queuing of messages to be sent
		 *  to clients or broadcasted to channels. They ensure that messages are sent in
		 *  the correct IRC format and manage the underlying network buffers.
		 * 
		 * @{
		 */
		void	sendReplyToClient(const std::string& nickname, int code, const std::string& msg);
		void	sendMessageToClient(const std::string& sender, const std::string& recipient, const std::string& msg);
		void	sendMessageToChannel(const std::string& sender, const std::string& channelName, const std::string& msg);
		void	sendRaw(Client* client, const std::string& rawLine);
		void	broadcastToChannel(const std::string& rawLine, const std::string& channelName, Client* exclude = NULL);
		/**
		 * @}
		 */

		/**
		 * @defgroup ChannelManagement Managing channels and clients
		 * @brief Methods for managing channels and their clients.
		 * @details These methods allow the server to add and remove channels, as well as
		 *  add and remove clients from channels. They ensure that the server maintains
		 *  an accurate representation of the current state of channels and their memberships.
		 * 
		 * @{
		 */
		void	addChannel(const std::string& channelName);
		void	removeChannel(const std::string& channelName);
		void	addClientToChannel(const std::string& channelName, Client* client);
		void	removeClientFromChannel(const std::string& channelName, Client* client);
		void	addOperatorToChannel(const std::string& channelName, Client* client);
		void	removeOperatorFromChannel(const std::string& channelName, Client* client);
		/**
		 * @}
		 */

		/**
		 * @defgroup LookupMethods Lookup methods for clients and channels
		 * @brief Methods for retrieving clients and channels by various identifiers.
		 * @details These methods provide convenient ways to look up clients by file descriptor
		 *  or nickname, and channels by name. They return pointers to the corresponding
		 *  objects or `nullptr` if the requested entity does not exist.
		 * 
		 * @{
		 */
		Client*		getClient(int fd) const;
		Client*		getClientByNickname(const std::string& nickname) const;
		bool		isNicknameInUse(const std::string& nickname) const;
		Channel*	getChannel(const std::string& channelName) const;
		bool		channelExists(const std::string& channelName) const;
		/**
		 * @}
		 */
};