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
	zmq::socket_t clients(context, ZMQ_ROUTER);

	std::stringstream bindURI;
	bindURI << "tcp://*:" << Options::LISTEN_PORT;
	clients.bind(bindURI.str().c_str());
	zmq::socket_t workers(context, ZMQ_DEALER);
	workers.bind("inproc://workers");

	/*
	 * Launch the worker threads
	 */
	std::vector<Server*> servers;
	for (int i = 0; i != std::thread::hardware_concurrency(); i++) {
		servers.push_back(new Server(merger, &context));
	}

	/*
	 * create a proxy from tcp input to the workers
	 */
	zmq::proxy(clients, workers, NULL);
	return EXIT_SUCCESS;
}
