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
		void	handleCommand(Client& client, const Command& cmd);
		void	handlePass(Client& client, const Command& cmd);
		void	handleNick(Client& client, const Command& cmd);
		void	handleUser(Client& client, const Command& cmd);
		void	handleJoin(Client& client, const Command& cmd);
		void	handlePrivmsg(Client& client, const Command& cmd);
		void	handleKick(Client& client, const Command& cmd);
		void	handleInvite(Client& client, const Command& cmd);
		void	handleTopic(Client& client, const Command& cmd);
		void	handleMode(Client& client, const Command& cmd);
};