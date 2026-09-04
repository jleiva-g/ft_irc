#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

class	Server;
class	Client;

class	Command {
	private:
		Command();
		Command(const Command&);
		Command& operator=(const Command&);
	public:
		static void	handleCommand(Client& client, Server& server, const string& raw);
};