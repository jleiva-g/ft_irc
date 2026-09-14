*This project has been created as part of the 42 curriculum by acesteve, emilgarc and jleiva-g.*

# ft_irc

## Description

`ft_irc` is a lightweight Internet Relay Chat (IRC) server written in C++98 as part of the 42 curriculum. The goal of the project is to implement the core server-side behavior of the IRC protocol: accept multiple TCP clients, register users, manage channels, validate commands, and route messages between clients.

The server uses a single event loop based on POSIX `poll()`. It keeps track of connected clients, nicknames, channels, channel members, operators, invitations, topics, keys, and user limits. Client sockets are non-blocking, and incoming and outgoing data is buffered so that IRC messages can be processed across partial network reads and writes.

### Implemented features

- Password-based client registration with `PASS`, `NICK`, and `USER`.
- IRC registration replies and validation of nicknames and channel names.
- Channel creation, joining, leaving, and multi-channel commands.
- Private and channel messages with `PRIVMSG`.
- Channel topics with `TOPIC`.
- Operator actions with `KICK` and `INVITE`.
- Channel modes `i` (invite-only), `t` (operator-only topic), `k` (key), `o` (operator), and `l` (user limit).
- `PING` and `PONG` handling.
- Numeric error replies for invalid commands, registration, channel access, and messaging operations.

The main components are:

- `Server`: owns the listening socket, event loop, connected clients, and channels.
- `Client`: stores connection state, registration data, and input/output buffers.
- `Channel`: stores members, operators, invitations, topic, key, and modes.
- `Command`: parses IRC lines and dispatches command handlers.

## Instructions

### Requirements

- A POSIX-compatible operating system such as Linux or macOS.
- `make`.
- A C++ compiler with C++98 support available as `c++`.

The Makefile enables `-Wall -Wextra -Werror -std=c++98` and does not require third-party libraries.

### Compilation

From the repository root, run:

```sh
make
```

This creates the `ircserv` executable and the intermediate object files in `obj/`.

Available Make targets:

```sh
make        # Build the server
make clean  # Remove object files
make fclean # Remove object files and the executable
make re     # Rebuild from scratch
```

### Execution

Start the server with a listening port and the password clients must use:

```sh
./ircserv <port> <password>
```

For example:

```sh
./ircserv 6667 secret
```

The server binds to all local IPv4 interfaces and keeps running in its event loop. A client must register with the configured password, nickname, and username before using channel and messaging commands.

You can connect with an IRC client such as HexChat, using `127.0.0.1` as the server address, port `6667`, and password `secret`. For a simple low-level connection check, `nc` can also be used:

```sh
nc 127.0.0.1 6667
PASS secret
NICK testuser
USER testuser 0 * :Test User
```

Commands sent manually must be terminated with CRLF (`\r\n`) when the client requires it.

### Documentation

The source files contain Doxygen comments. If Doxygen and Graphviz are installed, generate the HTML documentation from the repository root with:

```sh
doxygen Doxyfile
```

The generated documentation is written to `docs/html/` according to the current Doxygen configuration.

There is currently no automated test suite or `make test` target. Manual testing with an IRC client is recommended for multi-client, channel, mode, and messaging workflows.

## Resources

- [RFC 2812: Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812) - command syntax, registration, channels, messages, and numeric replies.
- [RFC 2810: Internet Relay Chat: Architecture](https://www.rfc-editor.org/rfc/rfc2810) - IRC architecture and server concepts.
- [POSIX `poll()` specification](https://pubs.opengroup.org/onlinepubs/9699919799/functions/poll.html) - multiplexing the listening socket and client connections.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - practical reference for sockets, `bind`, `listen`, `accept`, `recv`, and `send`.
- [C++ reference: containers](https://en.cppreference.com/w/cpp/container) - background for the `map`, `set`, and `vector` data structures used by the server.
- The repository's [Doxygen configuration](Doxyfile) and source comments - project-specific architecture and API documentation.

### AI usage

AI assistance was used for repository analysis and documentation. Specifically, it was used to inspect the Makefile and C++ sources, summarize the server architecture and currently implemented commands, verify the documented build command, and draft this README. The implementation remains the repository's C++ source code; this README records the project behavior based on that source rather than claiming AI-generated features.

## Authors

- [acesteve](https://github.com/acesteve)
- [emilgarc](https://github.com/emilgarc)
- [jleiva-g](https://github.com/jleiva-g)