#pragma once

#include <string>

#ifndef CHANNEL_LIMIT
#define CHANNEL_LIMIT 100
#endif

using std::string;

class	Client {
	private:
		int		fd;
		string	nickname;
		string	username;
		bool	passAccepted;
		bool	registered;
		string	recvBuffer;
		string	sendBuffer;
		int		channelCount;

	public:
		Client();
		int		getFd() const;
		string	getNickname() const;
		string	getUsername() const;
		bool	isPassAccepted() const;
		bool	isRegistered() const;
		string&	getSendBuffer();

		void	setFd(int fd);
		void	setNickname(const string& nickname);
		void	setUsername(const string& username);
		void	setPassAccepted(bool accepted);
		void	setRegistered(bool registered);

		void	appendRecvData(const string& data);
		bool	getOneCommandFromBuffer(string& comm);
		void	queueOneCommandToBuffer(const string& comm);

		bool	hasEnoughChannels() const;
		void	incrementChannelCount();
		void	decrementChannelCount();
};
