/**
 * @file Command.cpp
 * @brief Implements the `Command` class for handling IRC commands.
 * @details Implements the `Command` class, which parses and processes
 * IRC commands received from clients. It validates command arguments,
 * handles authentication and user-related commands, and dispatches
 * channel and messaging commands according to the IRC protocol.
 * 
 * @date 2026-09-04
 * @author Jesus Leiva Guerrero
 * @author Emilio Garcia Burgos
 * @author Lilith Estévez Boeta
 */

#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Numeric.hpp"

#include <cctype>
#include <sstream>
#include <cstdlib>
#include <cerrno>

using std::toupper;
using std::isalpha;
using std::isalnum;
using std::getline;
using std::stringstream;
using std::isdigit;
using std::strtoul;

/**
 * @brief Parses a raw IRC command into its command name and arguments.
 * @details Removes trailing CR/LF characters, extracts the command name,
 * converts it to uppercase, and separates its arguments according to the
 * IRC message format.
 * 
 * @param[in] raw Raw command received from the client.
 * @param[out] name Output command name, converted to uppercase.
 * @param[out] args Output vector containing the command arguments.
 */
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
	if (end == string::npos)
		name = line.substr(pos);
	else
		name = line.substr(pos, end - pos);
	pos = end;

	for (size_t i = 0; i < name.size(); ++i)
		name[i] = toupper(static_cast<unsigned char>(name[i]));

	while (pos < line.size()) {
		while (pos < line.size() && line[pos] == ' ')
			++pos;
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

/**
 * @brief Registers a client when all required authentication information is set.
 * @details A client is registered once its password has been accepted and
 * both a nickname and username have been set.
 * 
 * @param[in,out] client Client to register.
 * @param[in,out] server Server managing the client.
 */
static void	tryRegister(Client& client, Server& server) {
	if (client.isPassAccepted()
		&& !client.getNickname().empty()
		&& !client.getUsername().empty()
		&& !client.isRegistered())
		server.setRegistered(client);
}

/**
 * @brief Handles the IRC PASS command.
 * @details RFC 2812 syntax: PASS <password>
 * Validates the command arguments and attempts to authenticate the client
 * using the server connection password.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing the client.
 * @param[in] args Command arguments containing the connection password.
 */
static void	handlePass(Client& client, Server& server, const vector<string>& args) {
	if (args.size() > 1)
		return;
	if (args.empty()) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "PASS :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (client.isRegistered()) {
		server.sendCodeToClient(client, ERR_ALREADYREGISTERED, getNumericInfo(ERR_ALREADYREGISTERED).message);
		return;
	}
	if (!server.checkPassword(args[0])) {
		server.sendCodeToClient(client, ERR_PASSWDMISMATCH, getNumericInfo(ERR_PASSWDMISMATCH).message);
		return;
	}
	server.setPassAccepted(client);
	tryRegister(client, server);
}

/**
 * @brief Checks whether a character is a valid IRC nickname special character.
 * @details RFC 2812 nickname special characters are used in addition to
 * alphabetic and numeric characters when validating nicknames.
 * 
 * @param[in] c Character to check.
 * @return true if the character is a valid nickname special character,
 * false otherwise.
 */
static bool	isSpecial(const char& c) {
	return (c == '[' || c == ']' || c == '\\' || c == '`'
		|| c == '^' || c == '_' || c == '{' || c == '|');
}

/**
 * @brief Validates an IRC nickname.
 * @details RFC 2812 syntax:
 * nickname = ( letter / special ) *8( letter / digit / special / "-" )
 * Checks the nickname length and validates its characters according to
 * the IRC nickname format.
 * 
 * @param[in] nickname Nickname to validate.
 * @return true if the nickname is valid, false otherwise.
 */
static bool	isValidNickname(const string& nickname) {
	if (nickname.empty() || nickname.size() > 9)
		return false;

	unsigned char	c = static_cast<unsigned char>(nickname[0]);
	if (!isalpha(c) && !isSpecial(c))
		return false;

	for (size_t i = 1; i < nickname.size(); ++i) {
		c = static_cast<unsigned char>(nickname[i]);
		if (!isalnum(c) && !isSpecial(c) && c != '-')
			return false;
	}
	return true;
}

/**
 * @brief Handles the IRC NICK command.
 * @details RFC 2812 syntax: NICK <nickname>
 * Validates the requested nickname and handles the client's nickname
 * registration.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing the client.
 * @param[in] args Command arguments containing the requested nickname.
 */
static void	handleNick(Client& client, Server& server, const vector<string>& args) {
	if (args.empty() || args[0].empty()) {
		server.sendCodeToClient(client, ERR_NONICKNAMEGIVEN, getNumericInfo(ERR_NONICKNAMEGIVEN).message);
		return;
	}
	if (args.size() > 1)
		return;
	if (!isValidNickname(args[0])) {
		server.sendCodeToClient(client, ERR_ERRONEUSNICKNAME, args[0] + " " + getNumericInfo(ERR_ERRONEUSNICKNAME).message);
		return;
	}
	if (server.isNicknameInUse(args[0])) {
		server.sendCodeToClient(client, ERR_NICKNAMEINUSE,  args[0] + " " + getNumericInfo(ERR_NICKNAMEINUSE).message);
		return;
	}
	server.setNickname(client, args[0]);
	tryRegister(client, server);
}

/**
 * @brief Handles the IRC USER command.
 * @details RFC 2812 syntax: USER <user> <mode> <unused> <realname>
 * Validates the registration arguments and sets the client's username.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing the client.
 * @param[in] args Command arguments containing the USER parameters.
 */
static void	handleUser(Client& client, Server& server, const vector<string>& args) {
	if (args.size() < 4) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "USER :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (args.size() > 4)
		return;
	if (client.isRegistered()) {
		server.sendCodeToClient(client, ERR_ALREADYREGISTERED, getNumericInfo(ERR_ALREADYREGISTERED).message);
		return;
	}
	client.setUsername(args[0]);
	tryRegister(client, server);
}

/**
 * @brief Validates an IRC channel name.
 * @details RFC 2812 syntax: channel = 1*( "#" / "+" / "&" / "!" ) chanstring
 * Checks the channel prefix, maximum length, and prohibited characters.
 * 
 * @param[in] name Channel name to validate.
 * @return true if the channel name is valid, false otherwise.
 */
static bool isValidChannelName(const string& name)
{
	if (name.empty() || name.size() > 50
		|| (name[0] != '&' && name[0] != '#' && name[0] != '+' && name[0] != '!')
		|| name.find('\7') != string::npos || name.find(':') != string::npos)
		return false;
	return true;
}

/**
 * @brief Handles the IRC JOIN command.
 * @details RFC 2812 syntax: JOIN <channel>{,<channel>} [<key>{,<key>}]
 * Validates channel names and optional keys before requesting the server
 * to join the client to the requested channels.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing the client and channels.
 * @param[in] args Command arguments containing channel names and optional keys.
 */
static void	handleJoin(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		server.sendCodeToClient(client, ERR_NOTREGISTERED, getNumericInfo(ERR_NOTREGISTERED).message);
		return;
	}
	if (args.empty()) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "JOIN :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (args.size() > 2)
		return;
	if (args[0] == "0") {
		server.leaveAllChannels(client);
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
			server.sendCodeToClient(client, ERR_BADCHANMASK, name + " " + getNumericInfo(ERR_BADCHANMASK).message);
			continue;
		}
		if (server.channelExists(name) && server.isChannelFull(name)) {
			server.sendCodeToClient(client, ERR_CHANNELISFULL, name + " " + getNumericInfo(ERR_CHANNELISFULL).message);
			continue;
		}
		if (server.channelExists(name) && server.isChannelInviteOnly(name) && !server.isClientInvitedToChannel(client, name)) {
			server.sendCodeToClient(client, ERR_INVITEONLYCHAN, name + " " + getNumericInfo(ERR_INVITEONLYCHAN).message);
			continue;
		}
		if (server.channelExists(name) && server.isChannelKeyProtected(name) && !server.isChannelPass(name, key)) {
			server.sendCodeToClient(client, ERR_BADCHANNELKEY, name + " " + getNumericInfo(ERR_BADCHANNELKEY).message);
			continue;
		}
		if (server.clientHasEnoughChannels(client)) {
			server.sendCodeToClient(client, ERR_TOOMANYCHANNELS, name + " " + getNumericInfo(ERR_TOOMANYCHANNELS).message);
			continue;
		}
		server.joinChannel(client, name, key);
	}
}

/**
 * @brief Handles the IRC PRIVMSG command.
 * @details RFC 2812 syntax: PRIVMSG <target>{,<target>} <text>
 * Validates the message targets and ensures that message text is provided.
 * The message is then prepared for delivery to the requested users or channels.
 * 
 * @param[in,out] client Client sending the message.
 * @param[in,out] server Server managing clients and channels.
 * @param[in] args Command arguments containing message targets and text.
 */
static void	handlePrivmsg(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		server.sendCodeToClient(client, ERR_NOTREGISTERED, getNumericInfo(ERR_NOTREGISTERED).message);
		return;
	}
	if (args.empty() || args[0].empty()) {
		server.sendCodeToClient(client, ERR_NORECIPIENT, getNumericInfo(ERR_NORECIPIENT).message + " (PRIVMSG)");
		return;
	}
	if (args.size() > 2)
		return;
	if (args.size() < 2 || args[1].empty()) {
		server.sendCodeToClient(client, ERR_NOTEXTTOSEND, getNumericInfo(ERR_NOTEXTTOSEND).message);
		return;
	}

	stringstream	targets(args[0]);
	string			target;
	bool			hasTarget = false;
	while (getline(targets, target, ',')) {
		if (target.empty())
			continue;
		hasTarget = true;
		
		if (target[0] == '#' || target[0] == '&' || target[0] == '+' || target[0] == '!') {
			if (!server.channelExists(target)) {
				server.sendCodeToClient(client, ERR_NOSUCHCHANNEL, target + " " + getNumericInfo(ERR_NOSUCHCHANNEL).message);
				continue;
			}
			if (!server.isClientInChannel(client, target)) {
				server.sendCodeToClient(client, ERR_CANNOTSENDTOCHAN, target + " " + getNumericInfo(ERR_CANNOTSENDTOCHAN).message);
				continue;
			}
			server.sendMsgToChannel(client, target, args[1]);
		}
		else {
			if (!server.isNicknameInUse(target)) {
				server.sendCodeToClient(client, ERR_NOSUCHNICK, target + " " + getNumericInfo(ERR_NOSUCHNICK).message);
				continue;
			}
			server.sendMsgToClient(client, target, args[1]);
		}
	}
	if (!hasTarget) {
		server.sendCodeToClient(client, ERR_NORECIPIENT, getNumericInfo(ERR_NORECIPIENT).message + " (PRIVMSG)");
		return;
	}
}

/**
 * @brief Handles the IRC KICK command.
 * @details RFC 2812 syntax: KICK <channel>{,<channel>} <user>{,<user>} [<comment>]
 * Parses the target channels, users, and optional comment. A single channel
 * may target multiple users, while multiple channels require one user per
 * channel. Each channel/user pair is then passed to the server for validation
 * and processing.
 *
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing clients and channels.
 * @param[in] args Command arguments containing the channels, users,
 * and optional comment.
 */
static void	handleKick(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		server.sendCodeToClient(client, ERR_NOTREGISTERED, getNumericInfo(ERR_NOTREGISTERED).message);
		return;
	}
	if (args.size() < 2) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "KICK :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (args.size() > 3)
		return;

	stringstream	channels(args[0]);
	stringstream	users(args[1]);
	vector<string>	channelNames;
	vector<string>	nicknames;
	string			value;
	while (getline(channels, value, ',')) {
		if (!value.empty())
			channelNames.push_back(value);
	}
	while (getline(users, value, ',')) {
		if (!value.empty())
			nicknames.push_back(value);
	}
	if (channelNames.empty() || nicknames.empty())
		return;
	if (channelNames.size() > 1 && channelNames.size() != nicknames.size())
		return;

	string	comment;
	if (args.size() >= 3)
		comment = args[2];

	for (size_t i = 0; i < channelNames.size(); ++i) {
		if (!server.channelExists(channelNames[i])) {
			server.sendCodeToClient(client, ERR_NOSUCHCHANNEL, channelNames[i] + " " + getNumericInfo(ERR_NOSUCHCHANNEL).message);
			continue;
		}
		if (!server.isClientInChannel(client, channelNames[i])) {
			server.sendCodeToClient(client, ERR_NOTONCHANNEL, channelNames[i] + " " + getNumericInfo(ERR_NOTONCHANNEL).message);
			continue;
		}
		if (!server.isChannelOperator(channelNames[i], client.getNickname())) {
			server.sendCodeToClient(client, ERR_CHANOPRIVSNEEDED, channelNames[i] + " " + getNumericInfo(ERR_CHANOPRIVSNEEDED).message);
			continue;
		}
		if (!server.isNicknameInUse(nicknames[i])) {
			server.sendCodeToClient(client, ERR_NOSUCHNICK, nicknames[i] + " " + getNumericInfo(ERR_NOSUCHNICK).message);
			continue;
		}
		if (!server.isClientInChannel(*(server.getClientByNickname(nicknames[i])), channelNames[i])) {
			server.sendCodeToClient(client, ERR_USERNOTINCHANNEL, nicknames[i] + " " + channelNames[i] + " " + getNumericInfo(ERR_USERNOTINCHANNEL).message);
			continue;
		}
		server.kickClient(client, channelNames[i], nicknames[i], comment);
	}
}

/**
 * @brief Handles the IRC INVITE command.
 * @details RFC 2812 syntax: INVITE <nickname> <channel>
 * Validates the target nickname and channel before requesting the server
 * to invite the target client.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing clients and channels.
 * @param[in] args Command arguments containing the nickname and channel.
 */
static void	handleInvite(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		server.sendCodeToClient(client, ERR_NOTREGISTERED, getNumericInfo(ERR_NOTREGISTERED).message);
		return;
	}
	if (args.size() < 2) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "INVITE :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (args.size() > 2)
		return;

	if (!server.channelExists(args[1])) {
		server.sendCodeToClient(client, ERR_NOSUCHCHANNEL, args[1] + " " + getNumericInfo(ERR_NOSUCHCHANNEL).message);
		return;
	}
	if (!server.isNicknameInUse(args[0])) {
		server.sendCodeToClient(client, ERR_NOSUCHNICK, args[0] + " " + getNumericInfo(ERR_NOSUCHNICK).message);
		return;
	}
	if (!server.isClientInChannel(client, args[1])) {
		server.sendCodeToClient(client, ERR_NOTONCHANNEL, args[1] + " " + getNumericInfo(ERR_NOTONCHANNEL).message);
		return;
	}
	if (server.isChannelInviteOnly(args[1]) && !server.isChannelOperator(args[1], client.getNickname())) {
		server.sendCodeToClient(client, ERR_CHANOPRIVSNEEDED, args[1] + " " + getNumericInfo(ERR_CHANOPRIVSNEEDED).message);
		return;
	}
	if (server.isClientInChannel(*(server.getClientByNickname(args[0])), args[1])) {
		server.sendCodeToClient(client, ERR_USERONCHANNEL, args[0] + " " + args[1] + " " + getNumericInfo(ERR_USERONCHANNEL).message);
		return;
	}
	server.inviteClient(client, args[0], args[1]);
}

/**
 * @brief Handles the IRC TOPIC command.
 * @details RFC 2812 syntax: TOPIC <channel> [<topic>]
 * With no topic parameter, requests the current topic. With a topic
 * parameter, requests a topic change.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing the channel.
 * @param[in] args Command arguments containing the channel and optional topic.
 */
static void	handleTopic(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		server.sendCodeToClient(client, ERR_NOTREGISTERED, getNumericInfo(ERR_NOTREGISTERED).message);
		return;
	}
	if (args.empty()) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "TOPIC :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (args.size() > 2)
		return;
	if (!server.channelExists(args[0])) {
		server.sendCodeToClient(client, ERR_NOSUCHCHANNEL, args[0] + " " + getNumericInfo(ERR_NOSUCHCHANNEL).message);
		return;
	}
	if (!server.isClientInChannel(client, args[0])) {
		server.sendCodeToClient(client, ERR_NOTONCHANNEL, args[0] + " " + getNumericInfo(ERR_NOTONCHANNEL).message);
		return;
	}

	if (args.size() == 1) {
		server.sendTopic(client, args[0]);
		return;
	}
	if (server.isChannelTopicRestricted(args[0]) && !server.isChannelOperator(args[0], client.getNickname())) {
		server.sendCodeToClient(client, ERR_CHANOPRIVSNEEDED, args[0] + " " + getNumericInfo(ERR_CHANOPRIVSNEEDED).message);
		return;
	}
	server.setTopic(client, args[0], args[1]);
}

/**
 * @brief Checks whether an IRC channel mode is supported.
 * @details The mandatory ft_irc channel modes are i, t, k, o and l.
 * 
 * @param[in] c Mode character to check.
 * @return true if the mode is supported, false otherwise.
 */
static bool	isValidMode(char c) {
	return (c == 'i' || c == 't' || c == 'k' || c == 'o' || c == 'l');
}

/**
 * @brief Validates a channel user-limit value.
 * @details The value must contain only decimal digits and represent a
 * positive integer. A value of zero is not accepted as a user limit.
 * 
 * @param[in] value String representation of the requested user limit.
 * @return true if the value is a positive integer, false otherwise.
 */
static bool	isValidLimit(const string& value) {
	if (value.empty())
		return false;

	for (size_t i = 0; i < value.size(); ++i)
		if (!isdigit(static_cast<unsigned char>(value[i])))
			return false;

	errno = 0;
	unsigned long	limit = strtoul(value.c_str(), NULL, 10);
	return (errno != ERANGE && limit > 0);
}

/**
 * @brief Handles the IRC MODE command.
 * @details RFC 2812 syntax: MODE <channel> <modestring> [<mode parameters>]
 * Processes channel modes i, t, k, o and l, including their required
 * parameters, before requesting the corresponding server operation.
 * 
 * @param[in,out] client Client sending the command.
 * @param[in,out] server Server managing the channel.
 * @param[in] args Command arguments containing the channel, mode string,
 * and optional mode parameters.
 */
static void	handleMode(Client& client, Server& server, const vector<string>& args) {
	if (!client.isRegistered()) {
		server.sendCodeToClient(client, ERR_NOTREGISTERED, getNumericInfo(ERR_NOTREGISTERED).message);
		return;
	}
	if (args.empty() || (args.size() > 1 && args[1].empty())) {
		server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "MODE :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
		return;
	}
	if (!server.channelExists(args[0])) {
		server.sendCodeToClient(client, ERR_NOSUCHCHANNEL, args[0] + " " + getNumericInfo(ERR_NOSUCHCHANNEL).message);
		return;
	}
	if (args.size() == 1) {
		server.sendChannelModes(client, args[0]);
		return;
	}

	bool	set = true;
	size_t	argsIndex = 2;
	for (size_t i = 0; i < args[1].size(); ++i) {
		if (args[1][i] == '+') {
			set = true;
			continue;
		}
		if (args[1][i] == '-') {
			set = false;
			continue;
		}
		if (!isValidMode(args[1][i])) {
			server.sendCodeToClient(client, ERR_UNKNOWNMODE, string(args[1], i, 1) + " " + getNumericInfo(ERR_UNKNOWNMODE).message);
			continue;
		}
		if (!server.isClientInChannel(client, args[0])) {
			server.sendCodeToClient(client, ERR_NOTONCHANNEL, args[0] + " " + getNumericInfo(ERR_NOTONCHANNEL).message);
			continue;
		}
		if (!server.isChannelOperator(args[0], client.getNickname())) {
			server.sendCodeToClient(client, ERR_CHANOPRIVSNEEDED, args[0] + " " + getNumericInfo(ERR_CHANOPRIVSNEEDED).message);
			continue;
		}
		//server.checkHandle(client, args[0]);
		switch (args[1][i]) {
			case 'i':
				server.setInviteOnly(args[0], set);
				break;
			case 't':
				server.setTopicRestricted(args[0], set);
				break;
			case 'k':
				if (set) {
					if (argsIndex >= args.size()) {
						server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "MODE :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
						continue;
					}
					if (args[argsIndex].empty()) {
						++argsIndex;
						server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "MODE :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
						continue;
					}
					server.setChannelKey(args[0], args[argsIndex++]);
				} else {
					server.removeChannelKey(args[0]);
				}
				break;
			case 'o':
				if (argsIndex >= args.size()) {
					server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "MODE :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
					continue;
				}
				if (set)
					server.setChannelOperator(args[0], args[argsIndex++]);
				else
					server.removeChannelOperator(args[0], args[argsIndex++]);
				break;
			case 'l':
				if (set) {
					if (argsIndex >= args.size()) {
						server.sendCodeToClient(client, ERR_NEEDMOREPARAMS, "MODE :" + getNumericInfo(ERR_NEEDMOREPARAMS).message);
						continue;
					}
					if (!isValidLimit(args[argsIndex])) {
						++argsIndex;
						continue;
					}
					size_t	limit = static_cast<size_t>(strtoul(args[argsIndex++].c_str(), NULL, 10));
					server.setUserLimit(args[0], limit);
				} else {
					server.removeUserLimit(args[0]);
				}
				break;
		}
	}
	server.sendChannelModesToAll(client, args[0]);
}

/**
 * @brief Handles a complete IRC command received from a client.
 * @details Parses the raw IRC message, identifies the command, the arguments,
 * and dispatches them to the corresponding command handler.
 * 
 * @param[in,out] client Client that sent the command.
 * @param[in,out] server Server managing the server state.
 * @param[in] raw Raw IRC command received from the client.
 */
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
	else
		server.sendCodeToClient(client, ERR_UNKNOWNCOMMAND, name + " " + getNumericInfo(ERR_UNKNOWNCOMMAND).message);
}