#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

class	Server;
class	Client;

enum	Numeric {
	RPL_WELCOME = 001,
	ERR_NOSUCHNICK = 401,
	ERR_NOSUCHCHANNEL = 403,
	ERR_CANNOTSENDTOCHAN = 404,
	ERR_NORECIPIENT = 411,
	ERR_NOTEXTTOSEND = 412,
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
		Command();
		Command(const Command&);
		Command& operator=(const Command&);
	public:
		static void	handleCommand(Client& client, Server& server, const string& raw);
};