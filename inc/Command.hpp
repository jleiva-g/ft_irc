#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

class	Server;
class	Client;

enum	Numeric {
	RPL_WELCOME = 001,
	ERR_UNKNOWNCOMMAND = 421,
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONEUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_NOTREGISTERED = 451,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTERED = 462,
	ERR_PASSWDMISMATCH = 464
};

class	Command {
	private:
		string			_name;
		vector<string>	_args;
	public:
		Command(const string& raw);
		const string&			getName() const;
		const vector<string>&	getArgs() const;
		void	handleCommand(Client& client, Server& server);
		void	handlePass(Client& client, Server& server);
		void	handleNick(Client& client, Server& server);
		void	handleUser(Client& client);
		void	handleJoin(Client& client, Server& server);
		void	handlePrivmsg(Client& client, Server& server);
		void	handleKick(Client& client, Server& server);
		void	handleInvite(Client& client, Server& server);
		void	handleTopic(Client& client, Server& server);
		void	handleMode(Client& client, Server& server);
};