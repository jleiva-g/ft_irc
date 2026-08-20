#pragma once

#include <stdexcept>

using std::exception;
using std::string;

void manageErrorsFromSocket(int error, const char* file, int line);
void manageErrorsFromBind(int error, const char* file, int line);
void manageErrorsFromListen(int error, const char* file, int line);

struct PersonalizedException : public exception {
	protected:
		string	file;
		int		line;
		mutable string	errorMessage;
	public:
		PersonalizedException(const char* file, int line);
		~PersonalizedException() throw();
};

struct UnknownException : public PersonalizedException {
	UnknownException(const char* file, int line);
	const char* what() const throw();
};

struct NotEnoughPermissionsException : public PersonalizedException {
	NotEnoughPermissionsException(const char* file, int line);
	const char* what() const throw();
};

struct NotEnoughMemoryException : public PersonalizedException {
	NotEnoughMemoryException(const char* file, int line);
	const char* what() const throw();
};

struct NotEnoughFileDescriptorsException : public PersonalizedException {
	NotEnoughFileDescriptorsException(const char* file, int line);
	const char* what() const throw();
};

struct UnsupportedIPProtocolException : public PersonalizedException {
	UnsupportedIPProtocolException(const char* file, int line);
	const char* what() const throw();
};

struct PortInUseException : public PersonalizedException {
	PortInUseException(const char* file, int line);
	const char* what() const throw();
};

struct InvalidFileDescriptorException : public PersonalizedException {
	InvalidFileDescriptorException(const char* file, int line);
	const char* what() const throw();
};

struct AlredyLinkedFileDescriptorException : public PersonalizedException {
	AlredyLinkedFileDescriptorException(const char* file, int line);
	const char* what() const throw();
};

struct FileDescriptorIsNotSocketException : public PersonalizedException {
	FileDescriptorIsNotSocketException(const char* file, int line);
	const char* what() const throw();
};

struct SocketNotSupportListenException : public PersonalizedException {
	SocketNotSupportListenException(const char* file, int line);
	const char* what() const throw();
};
