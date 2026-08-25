#pragma once

#include <string>

using std::string;

class	Client {
	private:
		string	nickname;
		string	username;
		bool	passAccepted;
		bool	registered;
		string	recvBuffer;
		string	sendBuffer;

	public:
		Client();
		string&	getNickname() const;
		string&	getUsername() const;
		string&	getOutputBuffer();
		void	appendRecvData(const string& data);
		bool	getOneCommandFromBuffer(string& comm);
		void	queueOneCommandToBuffer(const string& comm);
};
