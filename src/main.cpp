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
		while (true) {
			
		}
	}
	catch (std::exception &e) { cerr << e.what() << endl; }
}