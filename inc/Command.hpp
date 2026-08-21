#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

class	Server;
class	Client;

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
		void	handleUser(Client& client, Server& server);
		void	handleJoin(Client& client, Server& server);
		void	handlePrivmsg(Client& client, Server& server);
		void	handleKick(Client& client, Server& server);
		void	handleInvite(Client& client, Server& server);
		void	handleTopic(Client& client, Server& server);
		void	handleMode(Client& client, Server& server);
};