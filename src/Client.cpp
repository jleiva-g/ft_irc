#include "Client.hpp"

Client::Client() : nickname(""), username(""), passAccepted(false), registered(false), recvBuffer(""), sendBuffer("") {}

void Client::appendRecvData(const string& data) { recvBuffer.append(data); }

bool Client::getOneCommandFromBuffer(string& comm) {
	size_t pos = recvBuffer.find("\r\n");
	if (pos == string::npos)
		return false;
	comm = recvBuffer.substr(0, pos);
	recvBuffer.erase(0, pos + 2);
	return true;
}

string& Client::getOutputBuffer() { return sendBuffer; }

void Client::queueOneCommandToBuffer(const string& comm) {
	sendBuffer.append(comm);
	sendBuffer.append("\r\n");
}
