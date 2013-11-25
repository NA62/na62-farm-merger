/*
 * main.cpp
 *
 *  Created on: 21.12.2010
 *      Author: kunzej
 */

#include <iostream>
#include <string>
#include <thread>
#include <zmq.hpp>

#include "options/Options.h"
#include "utils/Utils.h"

#include "dim/MonitorConnector.h"
#include "dim/CommandConnector.h"
#include "sockets/Server.hpp"

using namespace na62::merger;

int main(int argc, char* argv[]) {
	/*
	 * Read program parameters
	 */
	std::cout << "Initializing Options" << std::endl;
	Options::Initialize(argc, argv);

	Merger merger;
	MonitorConnector monitor(merger);

	CommandConnector commands(merger);
	commands.startThread();

	/*
	 * Open the tcp socket for the farm servers and an interior ipc socket to distribute the incoming data to the workers
	 */
	zmq::context_t context(1);

	/*
	 * Launch the worker threads
	 */
	std::vector<Server*> servers;
	for (uint threadNum = 0; threadNum != Options::NUMBER_OF_LISTEN_PORTS; threadNum++) {
		servers.push_back(new Server(merger, &context, threadNum));
	}

	while (true) {

	}
	return EXIT_SUCCESS;
}
