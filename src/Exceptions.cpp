#include "Exceptions.hpp"
#include <iostream>
#include <sstream>

using std::ostringstream;

void manageErrorsFromSocket(int error, const char* file, int line) {
	switch (error)
	{
		case EACCES:
			throw NotEnoughPermissionsException(file, line);
			break;
		case EAFNOSUPPORT:
			throw UnsupportedIPProtocolException(file, line);
			break;
		case ENOBUFS:
		case ENOMEM:
			throw NotEnoughMemoryException(file, line);
			break;
		case EMFILE:
		case ENFILE:
			throw NotEnoughFileDescriptorsException(file, line);
			break;
		default:
			throw UnknownException(file, line);
			break;
	}
}

void manageErrorsFromBind(int error, const char* file, int line) {
	switch (error)
	{
		case EADDRINUSE:
			throw PortInUseException(file, line);
			break;
		case EACCES:
			throw NotEnoughPermissionsException(file, line);
			break;
		case EBADF:
			throw InvalidFileDescriptorException(file, line);
			break;
		case EINVAL:
			throw AlredyLinkedFileDescriptorException(file, line);
			break;
		case ENOTSOCK:
			throw FileDescriptorIsNotSocketException(file, line);
			break;
		default:
			throw UnknownException(file, line);
			break;
	}
}

void manageErrorsFromListen(int error, const char* file, int line) {
	switch (error)
	{
		case EADDRINUSE:
			throw PortInUseException(file, line);
			break;
		case EBADF:
			throw InvalidFileDescriptorException(file, line);
			break;
		case ENOTSOCK:
			throw FileDescriptorIsNotSocketException(file, line);
			break;
		case EOPNOTSUPP:
			throw SocketNotSupportListenException(file, line);
			break;
		default:
			throw UnknownException(file, line);
			break;
	}
}

PersonalizedException::PersonalizedException(const char* file, int line) : file(file), line(line) {}
PersonalizedException::~PersonalizedException() throw() {}

UnknownException::UnknownException(const char* file, int line) : PersonalizedException(file, line) {}
const char* UnknownException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: There is a unknown problem with the server";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

NotEnoughPermissionsException::NotEnoughPermissionsException(const char* file, int line) : PersonalizedException(file, line) {}
const char* NotEnoughPermissionsException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The program doesn't have sufficient permissions";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

NotEnoughMemoryException::NotEnoughMemoryException(const char* file, int line) : PersonalizedException(file, line) {}
const char* NotEnoughMemoryException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The device doesn't have sufficient memory";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

NotEnoughFileDescriptorsException::NotEnoughFileDescriptorsException(const char* file, int line) : PersonalizedException(file, line) {}
const char* NotEnoughFileDescriptorsException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The device doesn't hace sufficient file descriptors";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

UnsupportedIPProtocolException::UnsupportedIPProtocolException(const char* file, int line) : PersonalizedException(file, line) {}
const char* UnsupportedIPProtocolException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The device doesn't support IPv4 direction's family";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

PortInUseException::PortInUseException(const char* file, int line) : PersonalizedException(file, line) {}
const char* PortInUseException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: Another socket is using the port yet";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

InvalidFileDescriptorException::InvalidFileDescriptorException(const char* file, int line) : PersonalizedException(file, line) {}
const char* InvalidFileDescriptorException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: Invalid file descriptor";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

AlredyLinkedFileDescriptorException::AlredyLinkedFileDescriptorException(const char* file, int line) : PersonalizedException(file, line) {}
const char* AlredyLinkedFileDescriptorException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The socket was linked previously";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

FileDescriptorIsNotSocketException::FileDescriptorIsNotSocketException(const char* file, int line) : PersonalizedException(file, line) {}
const char* FileDescriptorIsNotSocketException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The file descriptor used is not socket";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

SocketNotSupportListenException::SocketNotSupportListenException(const char* file, int line) : PersonalizedException(file, line) {}
const char* SocketNotSupportListenException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The file descriptor used is not socket";
	errorMessage = oss.str();
	return errorMessage.c_str();
}
