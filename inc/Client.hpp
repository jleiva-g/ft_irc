#pragma once

#include <string>

using std::string;

class	Client {
	private:
		int		_fd;
		string	_nickname;
		string	_username;
		bool	_passAccepted;
		bool	_registered;
		string	_recvBuffer;
		string	_sendBuffer;
	public:
		Client(int fd);
		int		getFd() const;
		string	getNickname() const;
		string	getUsername() const;
		void	appendRecvData(const string& data);
};