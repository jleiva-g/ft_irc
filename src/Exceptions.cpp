#include "Exceptions.hpp"
#include <iostream>
#include <sstream>

using std::ostringstream;

/**
 * @brief Converts errors from `socket()` into typed exceptions.
 * @details Groups equivalent resource and permission errors before throwing
 *  an exception that preserves the original source location.
 *
 * @param[in] error Error code returned through `errno`.
 * @param[in] file Source file where the error occurred.
 * @param[in] line Source line where the error occurred.
 * @throw PersonalizedException A typed exception matching the error code.
 */
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

/**
 * @brief Converts errors from `bind()` into typed exceptions.
 * @details Distinguishes address conflicts, invalid descriptors, and invalid
 *  socket types before throwing an exception with the source location.
 *
 * @param[in] error Error code returned through `errno`.
 * @param[in] file Source file where the error occurred.
 * @param[in] line Source line where the error occurred.
 * @throw PersonalizedException A typed exception matching the error code.
 */
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

/**
 * @brief Converts errors from `listen()` into typed exceptions.
 * @details Maps the system error to the most specific exception available;
 *  unrecognized errors become `UnknownException`.
 *
 * @param[in] error Error code returned through `errno`.
 * @param[in] file Source file where the error occurred.
 * @param[in] line Source line where the error occurred.
 * @throw PersonalizedException A typed exception matching the error code.
 */
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

/**
 * @brief Stores the source location associated with an exception.
 * @param[in] file Source file where the error occurred.
 * @param[in] line Source line where the error occurred.
 */
PersonalizedException::PersonalizedException(const char* file, int line) : file(file), line(line) {}

/** @brief Destroys the base exception without throwing. */
PersonalizedException::~PersonalizedException() throw() {}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
UnknownException::UnknownException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for an unclassified server error. */
const char* UnknownException::what() const throw() {
	// Rebuild the message so it always reflects the stored source location.
	ostringstream oss;
	oss << file << ":" << line << ": error: There is a unknown problem with the server";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
NotEnoughPermissionsException::NotEnoughPermissionsException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for a permission error. */
const char* NotEnoughPermissionsException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The program doesn't have sufficient permissions";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
NotEnoughMemoryException::NotEnoughMemoryException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for an insufficient-memory error. */
const char* NotEnoughMemoryException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The device doesn't have sufficient memory";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
NotEnoughFileDescriptorsException::NotEnoughFileDescriptorsException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for an exhausted file-descriptor limit. */
const char* NotEnoughFileDescriptorsException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The device doesn't hace sufficient file descriptors";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
UnsupportedIPProtocolException::UnsupportedIPProtocolException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for an unsupported IP protocol. */
const char* UnsupportedIPProtocolException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The device doesn't support IPv4 direction's family";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
PortInUseException::PortInUseException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for a port-in-use error. */
const char* PortInUseException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: Another socket is using the port yet";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
InvalidFileDescriptorException::InvalidFileDescriptorException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for an invalid file descriptor. */
const char* InvalidFileDescriptorException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: Invalid file descriptor";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
AlredyLinkedFileDescriptorException::AlredyLinkedFileDescriptorException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for an already-bound socket. */
const char* AlredyLinkedFileDescriptorException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The socket was linked previously";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
FileDescriptorIsNotSocketException::FileDescriptorIsNotSocketException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for a non-socket file descriptor. */
const char* FileDescriptorIsNotSocketException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The file descriptor used is not socket";
	errorMessage = oss.str();
	return errorMessage.c_str();
}

/** @param[in] file Source file where the error occurred. @param[in] line Source line where the error occurred. */
SocketNotSupportListenException::SocketNotSupportListenException(const char* file, int line) : PersonalizedException(file, line) {}

/** @brief Returns the message for a socket that cannot listen. */
const char* SocketNotSupportListenException::what() const throw() {
	ostringstream oss;
	oss << file << ":" << line << ": error: The socket not support listen";
	errorMessage = oss.str();
	return errorMessage.c_str();
}
