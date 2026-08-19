#pragma once

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

class	Server {
	private:
		int						_socketFd;
		int						_port;
		string					_password;
		vector<struct pollfd>	_pollFds;
		map<string, Client&>	_clients;
		map<string, Channel&>	_channels;
	public:
		Server(int port, const string& password);
		void	start();
		void	setupSocket();
		void	acceptClient();
		void	handleClient(int fd);
		void	removeClient(int fd);
		void	sendToClient(int fd, const string& msg);
		Client*	findClientByNickname(const string& nickname);
		Channel*	findChannel(const string& name);
		void	broadcastToChannel(const Channel& channel,
									int senderFd,
									const string& msg);
		bool	isRegistered(const Client& client) const;
		vector<string>	extractCommands(Client& client);
		bool	isChannelName(const string& name) const;
		bool	isValidNickname(const string& nickname) const;
};