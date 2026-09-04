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

using std::string;
using std::vector;
using std::map;

/**
 * @class Server
 * @brief Manages the IRC server and its client connections.
 *
 * @details Owns the listening socket, tracks connected clients and channels,
 *  and dispatches network events and client commands.
 */
class	Server {
	private:
		typedef map<int, Client*>::const_iterator			client_iterator;	///< Iterator for traversing the client map.
		typedef map<string, Client*>::const_iterator		nickname_iterator;	///< Iterator for traversing the nickname map.
		typedef map<string, Channel*>::const_iterator		channel_iterator;	///< Iterator for traversing the channel map.

		static const string		connectionAcceptMsg;	///< Welcome message queued for a newly accepted client.

		int								socketFd;				///< Listening socket file descriptor.
		int								port;					///< Port on which the server listens for connections.
		string							password;				///< Password required by the server.
		vector<pollfd>					pollFds;				///< File descriptors monitored with `poll()`.
		map<int, Client*>				clients;				///< Connected clients keyed by file descriptor.
		map<string, Client*>			nicknames;				///< Registered nicknames keyed by nickname string.
		map<string, Channel*>			channels;				///< Existing channels keyed by name.

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
		bool	checkPassword(const string& pass) const;

		void	setPassAccepted(Client& client);
		void	setNickname(Client& client, const string& nickname);
		void	setRegistered(Client& client);
		bool	isNicknameInUse(const string& nickname) const;


		void	leaveAllChannels(Client& client);
		void	joinChannel(Client& client, const string& channelName, const string& key = "");
		void	partChannel(Client& client, const string& channelName, const string& reason = "");

		// TODO: Mount messages to clients and channels.
		void	sendCodeToClient(Client& client, int code, const string& msg);
		void	notifyChannelChange(Client& client, const string& channelName, const string& msg);
		void	broadcastToChannel(const string& channelName, const string& msg, Client* excludeClient = NULL);
		void	sendMsgToClient(Client& client, const string& target, const string& msg);
		void	sendMsgToChannel(Client& client, const string& channelName, const string& msg);


		void	kickClient(Client& client, const string& channelName, const string& targetNickname, const string& reason);
		void	kickClient(Client& client, const string& channelName, const vector<string>& targetClients, const string& reason);
		void	inviteClient(Client& client, const string& targetNickname, const string& channelName);
		void	sendTopic(Client& client, const string& channelName);
		void	setTopic(Client& client, const string& channelName, const string& topic);


		void	sendChannelModes(Client& client, const string& channelName);
		bool	canModifyChannel(Client& client, const string& channelName) const;
		void	setInviteOnly(Client& client, const string& channelName, bool inviteOnly);
		void	setTopicRestricted(Client& client, const string& channelName, bool topicOpOnly);
		void	setChannelKey(Client& client, const string& channelName, const string& key);
		void	removeChannelKey(Client& client, const string& channelName);
		void	setChannelOperator(Client& client, const string& channelName, const string& targetNickname);
		void	removeChannelOperator(Client& client, const string& channelName, const string& target);
		void	setUserLimit(Client& client, const string& channelName, size_t limit);
		void	removeUserLimit(Client& client, const string& channelName);
};