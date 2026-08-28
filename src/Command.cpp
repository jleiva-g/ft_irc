#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <cctype>
#include <sstream>

using std::toupper;
using std::isalpha;
using std::isalnum;
using std::getline;
using std::stringstream;

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
	for (size_t i = 0; i < name.size(); ++i)
		name[i] = std::toupper(static_cast<unsigned char>(name[i]));
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

static void	tryRegister(Client& client, Server& server) {
	if (client.isPassAccepted()
		&& !client.getNickname().empty()
		&& !client.getUsername().empty()
		&& !client.isRegistered())
		server.setRegistered(client);
}

// PASS <password>
static void	handlePass(Client& client, const Server& server, const vector<string>& args) {
	if (args.size() > 1)
		return;
	if (args.empty()) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (client.isRegistered()) {
		// 462 ERR_ALREADYREGISTRED
		return;
	}
	// pass match?	464 ERR_PASSWDMISMATCH
	server.setPassAccepted(client, args[0]);
	tryRegister(client, server);
}

static bool	isSpecial(const char& c) {
	return (c == '[' || c == ']' || c == '\\' || c == '`'
		|| c == '^' || c == '_' || c == '{' || c == '|');
}

static bool	isValidNickname(const string& nickname) {
	if (nickname.empty() || nickname.size() > 9)
		return false;

	unsigned char	c = static_cast<unsigned char>(nickname[0]);
	if (!isalpha(c) && !isSpecial(c))
		return false;

	for (size_t i = 1; i < nickname.size(); i++) {
		c = static_cast<unsigned char>(nickname[i]);
		if (!isalnum(c) && !isSpecial(c) && c != '-')
			return false;
	}
	return true;
}

// NICK <nickname>
static void	handleNick(Client& client, Server& server, const vector<string>& args) {
	if (args.empty() || args[0].empty()) {
		// 431 ERR_NONICKNAMEGIVEN
		return;
	}
	if (args.size() > 1)
		return;
	if (!isValidNickname(args[0])) {
		// 432 ERR_ERRONEUSNICKNAME
		return;
	}
	// is nickname taken? 433 ERR_NICKNAMEINUSE
	// server.setNickname(client, args[0]);
	tryRegister(client, server);
}

// USER <username> <mode> <unused> <realname>
static void	handleUser(Client& client, const Server& server, const vector<string>& args) {
	if (args.size() < 4) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (args.size() > 4)
		return;
	if (client.isRegistered()) {
		// 462 ERR_ALREADYREGISTRED
		return;
	}
	client.setUsername(args[0]);
	tryRegister(client, server);
}

static bool isValidChannelName(const string& name)
{
	if (name.empty() || name.size() > 50
		|| (name[0] != '&' && name[0] != '#' && name[0] != '+' && name[0] != '!')
		|| name.find('\7') != string::npos || name.find(':') != string::npos)
		return false;
	return true;
}

// JOIN <channel>{,<channel>} [<key>{,<key>}]
static void	handleJoin(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.empty()) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (args.size() > 2)
		return;
	if (args[0] == "0") {
		// server.leaveAllChannels(client);
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
		if (!isValidChannelName(name)) {
			// 476 ERR_BADCHANMASK
			continue;
		}
		// channel limit?	405 ERR_TOOMANYCHANNELS
		// +i?				473 ERR_INVITEONLYCHAN
		// wrong key?		475 ERR_BADCHANNELKEY
		// user limit?		471 ERR_CHANNELISFULL
		// channel exists?	create if needed
		// server.joinChannel(client, name, key);
	}
}

// PRIVMSG <target>{,<target>} <text>
static void	handlePrivmsg(Client& client, const Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.empty() || args[0].empty()) {
		// 411 ERR_NORECIPIENT
		return;
	}
	if (args.size() > 2)
		return;
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
		// if (target[0] == '#' || target[0] == '&'
		// 	|| target[0] == '+' || target[0] == '!')
		// 	// channel exists? 403 ERR_NOSUCHCHANNEL
		// 	// server.sendMsgToChannel(client, target, args[1])
		// else
		// 	// client exists? 401 ERR_NOSUCHNICK
		// 	// server.sendMesgToClient(client, target, args[1])
	}
	if (!hasTarget) {
		// 411 ERR_NORECIPIENT
		return;
	}
}

// KICK <channel> <user>{,<user>} [<comment>]
static void	handleKick(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.size() < 2) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (args.size() > 3)
		return;

	stringstream	users(args[1]);
	string			nickname;
	vector<string>	nicknames;
	string			comment;
	if (args.size() >= 3)
		comment = args[2];
	while (getline(users, nickname, ',')) {
		if (!nickname.empty())
			nicknames.push_back(nickname);
	}
	if (nicknames.empty())
		return;
	// channel exists?			403 ERR_NOSUCHCHANNEL
	// is kicker in channel?	442 ERR_NOTONCHANNEL
	// is kicker op?			482 ERR_CHANOPRIVSNEEDED
	// target exists?			401 ERR_NOSUCHNICK
	// is target in channel?	441 ERR_USERNOTINCHANNEL
	// server.kickClient(client, args[0], nicknames, comment);
}

// INVITE <nickname> <channel>
static void	handleInvite(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.size() < 2) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (args.size() > 2)
		return;

	// channel exists?		403 ERR_NOSUCHCHANNEL
	// target exists?		401 ERR_NOSUCHNICK
	// inviter in channel?	442 ERR_NOTONCHANNEL
	// inviter op if +i?	482 ERR_CHANOPRIVSNEEDED
	// target in channel?	443 ERR_USERONCHANNEL
	// server.inviteClient(client, args[0], args[1])
}

// TOPIC <channel> [<topic>]
static void	handleTopic(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		// 451 ERR_NOTREGISTERED
		return;
	}
	if (args.empty()) {
		// 461 ERR_NEEDMOREPARAMS
		return;
	}
	if (args.size() > 2)
		return;

	if (args.size() == 1) {
		// channel exists?		403 ERR_NOSUCHCHANNEL
		// client in channel?	442 ERR_NOTONCHANNEL
		// server.sendTopic(client, channel);
		return;
	}
	// channel exists?		403 ERR_NOSUCHCHANNEL
	// client in channel?	442 ERR_NOTONCHANNEL
	// client op if +t?		482 ERR_CHANOPRIVSNEEDED
	// broadcast topic
	// server.setTopic(client, args[0], args[1]);
}

// MODE <channel> <mode> [<mode parameters>...]
static void	handleMode(Client& client, Server& server, const vector<string>& args) {
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
		handleUser(client, server, args);
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