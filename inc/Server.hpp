#pragma once

/**
 * @file Server.hpp
 * @brief Declaration of the IRC server class.
 * @date 2026-07-19
 * @author jleiva-g
 * @author emilgar
 * @author acesteve
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
		typedef map<string, Channel*>::const_iterator	channel_iterator;	///< Iterator for traversing the channel map.

		static const string		connectionAcceptMsg;	///< Welcome message queued for a newly accepted client.

		int						socketFd;				///< Listening socket file descriptor.
		int						port;					///< Port on which the server listens for connections.
		string					password;				///< Password required by the server.
		vector<pollfd>			pollFds;				///< File descriptors monitored with `poll()`.
		map<int, Client*>		clients;				///< Connected clients keyed by file descriptor.
		map<string, Channel*>	channels;				///< Existing channels keyed by name.

		void			mainLoop();
		void			proccessPollfd(int i);
		void			proccessIn(int fd);
		void			proccessOut(pollfd& poll);
		vector<string>	proccessCommand(const string& cmdLine);
		void			queueMessage(pollfd& poll, const string& msg);

	public:
		Server(int port, const string& password);
		~Server();
		void			start();
		void			acceptClient();
		Client*			findClientByNickname(const string& nickname);
		Channel*		findChannel(const string& name);
		void			broadcastToChannel(const Channel& channel, int senderFd, const string& msg);
		bool			isRegistered(const Client& client) const;
		bool			isChannelName(const string& name) const;
		bool			isValidNickname(const string& nickname) const;
};