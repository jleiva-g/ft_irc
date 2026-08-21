#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

class	Client;

class	Command {
	private:
		string			_name;
		vector<string>	_args;
	public:
		Command(const string& raw);
		const string&			getName() const;
		const vector<string>&	getArgs() const;
		void	handleCommand(Client& client);
		void	handlePass(Client& client);
		void	handleNick(Client& client);
		void	handleUser(Client& client);
		void	handleJoin(Client& client);
		void	handlePrivmsg(Client& client);
		void	handleKick(Client& client);
		void	handleInvite(Client& client);
		void	handleTopic(Client& client);
		void	handleMode(Client& client);
};