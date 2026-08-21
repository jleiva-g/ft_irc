#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

Command::Command(const string& raw) {
	string	line = raw;
	while (!line.empty()
		&& (line[line.size() - 1] == '\n'
		|| line[line.size() - 1] == '\r'))
		line.erase(line.size() - 1);

	size_t	pos = line.find_first_not_of(' ');
	if (pos == string::npos)
		return;

	size_t	end = line.find(' ', pos);
	if (end == string::npos) {
		_name = line.substr(pos);
		return;
	}

	_name = line.substr(pos, end - pos);
	pos = end;

	while (pos < line.size()) {
		while (pos < line.size() && line[pos] == ' ')
			pos++;
		if (pos >= line.size())
			break;
		if (line[pos] == ':') {
			_args.push_back(line.substr(pos + 1));
			break;
		}
		end = line.find(' ', pos);
		if (end == string::npos) {
			_args.push_back(line.substr(pos));
			break;
		}
		_args.push_back(line.substr(pos, end - pos));
		pos = end;
	}
}

const string&	Command::getName() const {
	return _name;
}

const vector<string>&	Command::getArgs() const {
	return _args;
}

void	Command::handleCommand(Client& client, Server& server) {
	if (_name == "PASS")
		handlePass(client, server);
	else if (_name == "NICK")
		handleNick(client, server);
	else if (_name == "USER")
		handleUser(client);
	else if (_name == "JOIN")
		handleJoin(client, server);
	else if (_name == "PRIVMSG")
		handlePrivmsg(client, server);
	else if (_name == "KICK")
		handleKick(client, server);
	else if (_name == "INVITE")
		handleInvite(client, server);
	else if (_name == "TOPIC")
		handleTopic(client, server);
	else if (_name == "MODE")
		handleMode(client, server);
}

void	Command::handlePass(Client& client, Server& server) {
	if (_args.size() != 1) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (_args[0] != server.getPassword()) {
		// 464 ERR_PASSWDMISMATCH
		return;
	}
	client.setPassAccepted(true);
	// tryRegister()
}

void	Command::handleNick(Client& client, Server& server) {
	if (_args.size() != 1 || _args[0].empty()) {
		// 431 ERR_NONICKNAMEGIVEN
		return;
	}
	if (server.isNicknameTaken(_args[0])) {
		// 433 ERR_NICKNAMEINUSE
		return;
	}
	client.setNickname(_args[0]);
	// tryRegister()
}

void	Command::handleUser(Client & client) {
	if (_args.size() != 4) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (client.isRegistered()) {
		// ERR_ALREADYREGISTRED
		return;
	}
	client.setUsername(_args[0]);
	// tryRegister()
}