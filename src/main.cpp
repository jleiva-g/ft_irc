/**
 * @file main.cpp
 * @brief Entry point for the IRC server application.
 * @details Implements the `main()` function, which parses command-line arguments,
 *  initializes the `Server` instance, and starts the server's main event loop.
 * 
 * @date 2026-07-19
 * @author Jesus Leiva Guerrero
 * @author Emilio Garcia Burgos
 * @author Lilith Estévez Boeta
 */

 /**
 * @mainpage FT IRC
 *
 * @section sec_overview Overview
 *
 * **ft_irc** (executable: `ircserv`) is a TCP/IPv4 IRC server written in
 * C++98, developed as part of the 42 curriculum. Its goal is to provide the
 * core infrastructure to accept multiple simultaneous clients, multiplex
 * their connections with `poll()`, and route IRC messages between users.
 *
 * This page documents the project **as currently implemented**, not as
 * announced by the README. Where a feature is only declared in a header but
 * not backed by working code, it is explicitly marked as *planned* or
 * *pending* below, so the documentation stays trustworthy as the project
 * evolves.
 *
 * @section sec_architecture Architecture
 *
 * The design revolves around a single ::Server instance that owns every
 * client and channel and drives the connection lifecycle:
 *
 * - ::Server owns the listening socket, the `poll()` set, and indexes
 *   clients by file descriptor and by nickname.
 * - ::Client holds the state of one connection: its receive/send buffers
 *   and its registration state (nickname, username, PASS/registration
 *   flags).
 * - ::Channel models the state of a single IRC channel (topic, key, modes,
 *   members and operators).
 * - ::Command is intended as the parser/dispatcher for IRC commands
 *   (`PASS`, `NICK`, `USER`, `JOIN`, `PRIVMSG`, `KICK`, `INVITE`, `TOPIC`,
 *   `MODE`, ...).
 * - The `Exceptions` module (see ::PersonalizedException and its
 *   subclasses) turns socket/bind/listen `errno` values into typed,
 *   file-and-line-aware exceptions.
 *
 * No custom namespace is used; the codebase relies on global
 * `using std::string / map / vector / set` declarations.
 *
 * @section sec_flow Execution Flow
 *
 * -# `main()` validates exactly two arguments (`<port> <password>`),
 *    parses the port with `strtol()` and rejects non-numeric, overflowing,
 *    negative or out-of-range (> 65535) values.
 * -# Server::Server() is constructed with the port and password, and
 *    Server::start() creates an `AF_INET`/`SOCK_STREAM` socket, binds it to
 *    `INADDR_ANY`, calls `listen()`, and registers it in `pollFds`.
 * -# Server::mainLoop() blocks on `poll()` and dispatches every event
 *    through Server::proccessPollfd() *(spelling kept as-is in the public
 *    API — do not "correct" it without changing the API itself)*.
 * -# Server::proccessIn() either `accept()`s a new client (setting its
 *    descriptor `O_NONBLOCK` and registering it for `POLLIN`) or receives
 *    up to 1024 bytes from an existing one, appending them to
 *    Client::recvBuffer.
 * -# Complete commands are extracted from the buffer on CRLF boundaries via
 *    Client::getOneCommandFromBuffer(). In the current sources these
 *    extracted lines are only printed to `stdout` — they are **not yet**
 *    routed to a ::Command object.
 * -# Server::proccessOut() flushes Client::sendBuffer with `send()`,
 *    tolerating partial writes and toggling `POLLOUT` accordingly.
 * -# On `POLLHUP`, `POLLERR`, `POLLNVAL` or a zero-byte `recv()`,
 *    Server::removeClient() unregisters the nickname, destroys the
 *    ::Client, removes its `pollfd` and closes the descriptor.
 *
 * @section sec_status Implementation Status
 *
 * @subsection sec_status_done Working today
 *
 * - IPv4/TCP server accepting multiple clients via `poll()`.
 * - Non-blocking client sockets.
 * - CRLF-framed receive buffering and partial-send-aware output buffering.
 * - Client bookkeeping by descriptor and by nickname, including nickname
 *   change and duplicate-nickname detection (numeric reply `433`).
 * - Construction of numeric server replies and of private `PRIVMSG` text.
 * - Cleanup of disconnected clients.
 * - Typed translation of `socket()`/`bind()`/`listen()` errors.
 *
 * @subsection sec_status_pending Declared but not demonstrably implemented
 *
 * - Effective `PASS` authentication and `NICK`/`USER` registration.
 * - `JOIN` and channel management in general.
 * - Full `PRIVMSG` handling and message broadcast to channels.
 * - `KICK`, `INVITE`, `TOPIC`, `MODE` and the associated channel modes,
 *   keys, user limits, invites and operator privileges.
 * - ::Command::handleCommand() and its per-command handlers.
 * - The ::Channel class (`src/Channel.cpp` currently only includes its
 *   header).
 * - The `Utils` module (`inc/Utils.hpp` / `src/Utils.cpp` are empty).
 * - A fully ordered shutdown of ::Server (the destructor does not show an
 *   explicit `close()` of the listening socket).
 *
 * @section sec_build Building and Running
 *
 * The project builds with a plain Makefile — there is no CMake or other
 * external dependency manager.
 *
 * Requirements:
 * - A C++98-capable compiler available as `c++`.
 * - POSIX headers/APIs: sockets, `poll`, `fcntl`, `recv`, `send`, `close`,
 *   `errno`.
 * - `make`.
 * - Doxygen 1.9.1 and Graphviz/`dot` to regenerate this documentation
 *   (call graphs are enabled in the `Doxyfile`).
 *
 * Compilation flags: `-Wall -Wextra -Werror -std=c++98`.
 *
 * | Target        | Effect                                            |
 * |---------------|----------------------------------------------------|
 * | `make`        | Builds objects in `obj/` and links `ircserv`.     |
 * | `make clean`  | Removes `obj/`.                                   |
 * | `make fclean` | Removes `obj/` and the `ircserv` binary.          |
 * | `make re`     | Runs `fclean` then `make`.                        |
 *
 * Usage:
 * @code
 * ./ircserv <port> <password>
 * ./ircserv 6667 secret
 * @endcode
 *
 * The server binds to all local interfaces (`INADDR_ANY`) and runs a
 * blocking event loop; there is currently no config-file or environment
 * variable based configuration.
 *
 * @section sec_testing Testing
 *
 * There is no automated test suite or `make` test target.
 *
 * @section sec_limitations Known Limitations
 *
 * - `Channel`, `Command` and `Utils` have declared headers but empty `.cpp`
 *   files.
 * - `main()` contains a `while (true)` after `server.start()`, but
 *   `start()` already enters `mainLoop()` itself, so that loop is
 *   unreachable while the server is running.
 * - The welcome reply is sent via `sendReplyToClient("", 001, ...)` before
 *   the client has a registered nickname; this needs revisiting.
 * - Server::sendReplyToClient() throws `std::runtime_error` if the target
 *   nickname does not exist.
 * - Server::sendMessageToChannel() is declared but has no visible
 *   implementation in `src/Server.cpp`.
 * - Some IRC prefixes/replies still use placeholder values such as
 *   `irc.miservidor.com`; these are not real configuration.
 * - `inc/Client.hpp`, `inc/Server.hpp`, `src/Client.cpp`, `src/Server.cpp`,
 *   `src/main.cpp` and the `Doxyfile` have local uncommitted changes at the
 *   time of writing; this page reflects the observed working state, not a
 *   single tagged commit.
 *
 * @section sec_meta Project Metadata
 *
 * @author Jesus Leiva Guerrero (jleiva-g)
 * @author Emilio Garcia Burgos (emilgarc)
 * @author Lilith Estevez Boeta (acesteve)
 * @version 1.0.0
 * @date 2026-07-19 (Server, Client, main) / 2026-07-20 (Exceptions)
 *
 * @note License and contribution guidelines are **TODO**: no `LICENSE` or
 * `CONTRIBUTING.md` file is currently tracked in the repository.
 *
 * @see Server
 * @see Client
 * @see Channel
 * @see Command
 * @see PersonalizedException
 */


#include "Server.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <stdexcept>

using std::cerr;
using std::endl;

int	main(int ac, char* av[]) {
	if (ac != 3)
		return 1;
	
	char* end;
	long port = std::strtol(av[1], &end, 10);
	if (av[1] == end || *end != 0 || errno == ERANGE || port > 65535 || port < 0)
		return 1;
	Server server(port, av[2]);
	try {
		server.start();
	}
	catch (std::exception &e) { cerr << e.what() << endl; }
}
