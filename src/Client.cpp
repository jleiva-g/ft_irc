#include "Client.hpp"

Client::Client() : fd(-1), nickname(""), username(""), passAccepted(false), registered(false), recvBuffer(""), sendBuffer(""), channelCount(0) {}

int Client::getFd() const { return fd; }
void Client::setFd(int newFd) { fd = newFd; }

string Client::getNickname() const { return nickname; }
void Client::setNickname(const string& newNickname) { nickname = newNickname; }

string Client::getUsername() const { return username; }
void Client::setUsername(const string& newUsername) { username = newUsername; }

bool Client::isPassAccepted() const { return passAccepted; }
void Client::setPassAccepted(bool accepted) { passAccepted = accepted; }

bool Client::isRegistered() const { return registered; }
void Client::setRegistered(bool isRegistered) { registered = isRegistered; }

string& Client::getSendBuffer() { return sendBuffer; }

void Client::appendRecvData(const string& data) { recvBuffer.append(data); }

bool Client::getOneCommandFromBuffer(string& comm) {
	size_t pos = recvBuffer.find("\r\n");
	if (pos == string::npos)
		return false;
	comm = recvBuffer.substr(0, pos);
	recvBuffer.erase(0, pos + 2);
	return true;
}

void Client::queueOneCommandToBuffer(const string& comm) {
	sendBuffer.append(comm);
	sendBuffer.append("\r\n");
}

void Client::incrementChannelCount() { ++channelCount; }

bool Client::hasEnoughChannels() const { return channelCount >= CHANNEL_LIMIT; }
