#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <sstream>

using std::stringstream;
using std::getline;

static void	parseCommand(const string& raw, string& name, vector<string>& args) {
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
		name = line.substr(pos);
		return;
	}

	name = line.substr(pos, end - pos);
	pos = end;

	while (pos < line.size()) {
		while (pos < line.size() && line[pos] == ' ')
			pos++;
		if (pos >= line.size())
			break;
		if (line[pos] == ':') {
			args.push_back(line.substr(pos + 1));
			break;
		}
		end = line.find(' ', pos);
		if (end == string::npos) {
			args.push_back(line.substr(pos));
			break;
		}
		args.push_back(line.substr(pos, end - pos));
		pos = end;
	}
}

static void	tryRegister(Client& client) {
	if (client.isPassAccepted()
		&& !client.getNickname().empty()
		&& !client.getUsername().empty()
		&& !client.isRegistered()) {
		client.setRegistered(true);
		// 001 RPL_WELCOME
	}
}

static void	handlePass(Client& client, const Server& server, const vector<string>& args) {
	if (args.size() != 1) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (args[0] != server.getPassword()) {
		// 464 ERR_PASSWDMISMATCH
		return;
	}
	client.setPassAccepted(true);
	tryRegister(client);
}

static void	handleNick(Client& client, const Server& server, const vector<string>& args) {
	if (args.size() != 1 || args[0].empty()) {
		// 431 ERR_NONICKNAMEGIVEN
		return;
	}
	if (server.isValidNickname(args[0])) {
		// 432 ERR_ERRONEUSNICKNAME
		return;
	}
	if (server.isNicknameTaken(args[0])) {
		// 433 ERR_NICKNAMEINUSE
		return;
	}
	client.setNickname(args[0]);
	tryRegister(client);
}

static void	handleUser(Client& client, const vector<string>& args) {
	if (args.size() != 4) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (client.isRegistered()) {
		// 462 ERR_ALREADYREGISTRED
		return;
	}
	client.setUsername(args[0]);
	tryRegister(client);
}

static void	handleJoin(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.size() < 1) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}

	stringstream	channels(args[0]);
	stringstream	keys;
	string			name;
	string			key;
	if (args.size() > 1)
		keys.str(args[1]);

	while (getline(channels, name, ',')) {
		key.clear();
		if (args.size() > 1)
			getline(keys, key, ',');
		if (name.empty())
			continue;
		// join channel
	}
}

static void	handlePrivmsg(Client& client, const Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.empty() || args[0].empty()) {
		// 411 ERR_NORECIPIENT
		return;
	}
	if (args.size() < 2 || args[1].empty()) {
		// 412 ERR_NOTEXTTOSEND
		return;
	}

	stringstream	targets(args[0]);
	string			target;
	bool			hasTarget = false;
	while (getline(targets, target, ',')) {
		if (target.empty())
			continue;
		hasTarget = true;
		if (target[0] == '#') {
			if (!server.channelExists(target)) {
				// 403 ERR_NOSUCHCHANNEL
				continue;
			}
			// sendToChannel()
		} else {
			if (!server.clientExists(target)) {
				// 401 ERR_NOSUCHNICK
				continue;
			}
			// sendToClient()
		}
	}
	if (!hasTarget) {
		// 411 ERR_NORECIPIENT
		return;
	}
}

static void	handleKick(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.size() < 2) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (!server.channelExists(args[0])) {
		// 403 ERR_NOSUCHCHANNEL
		return;
	}
	if (!server.isClientInChannel(client, args[0])) {
		// 442 ERR_NOTONCHANNEL
		return;
	}
	if (!server.isChannelOp(client, args[0])) {
		// 482 ERR_CHANOPRIVSNEEDED
		return;
	}

	stringstream	users(args[1]);
	string			nickname;
	string			comment;
	if (args.size() >= 3)
		comment = args[2];
	while (getline(users, nickname, ',')) {
		if (nickname.empty())
			continue;
		if (!server.clientExists(nickname)) {
			// 401 ERR_NOSUCHNICK
			continue;
		}
		if (!server.isClientInChannel(nickname, args[0])) {
			// 441 ERR_USERNOTINCHANNEL
			continue;
		}
		// server.kickClient(client, nickname, args[0], comment);
	}
}

void	Command::handleCommand(Client& client, Server& server, const string& raw) {
	string			name;
	vector<string>	args;
	parseCommand(raw, name, args);

	if (name == "PASS")
		handlePass(client, server, args);
	else if (name == "NICK")
		handleNick(client, server, args);
	else if (name == "USER")
		handleUser(client, args);
	else if (name == "JOIN")
		handleJoin(client, server, args);
	else if (name == "PRIVMSG")
		handlePrivmsg(client, server, args);
	else if (name == "KICK")
		handleKick(client, server, args);
	else if (name == "INVITE")
		handleInvite(client, server, args);
	else if (name == "TOPIC")
		handleTopic(client, server, args);
	else if (name == "MODE")
		handleMode(client, server, args);
	// else
		// 421 ERR_UNKNOWNCOMMAND
}