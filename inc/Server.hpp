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
		int						socketFd;
		int						port;
		string					password;
		vector<pollfd>			pollFds;
		map<string, Client*>	clients;
		map<string, Channel*>	channels;

	public:
		Server(int port, const string& password);
		void			start();
		void			acceptClient();
		void			handleClient(int fd);
		void			removeClient(int fd);
		void			sendToClient(int fd, const string& msg);
		Client*			findClientByNickname(const string& nickname);
		Channel*		findChannel(const string& name);
		void			broadcastToChannel(const Channel& channel, int senderFd, const string& msg);
		bool			isRegistered(const Client& client) const;
		vector<string>	extractCommands(Client& client);
		bool			isChannelName(const string& name) const;
		bool			isValidNickname(const string& nickname) const;
};