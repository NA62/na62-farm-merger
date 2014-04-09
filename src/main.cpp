/*
 * main.cpp
 *
 *  Created on: 21.12.2010
 *      Author: kunzej
 */

#include <sys/types.h>
#include <unistd.h>
#include <zmq.h>
#include <zmq.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "dim/CommandConnector.h"
#include "dim/MonitorConnector.h"
#include "merger/Merger.hpp"
#include "options/Options.h"
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

	zmq::socket_t frontEnd(context, ZMQ_PULL);
	zmq::socket_t backEnd(context, ZMQ_PUSH);

	std::stringstream bindURI;
	bindURI << "tcp://*:" << Options::LISTEN_PORT;

	std::cout << "Opening ZMQ socket " << bindURI.str() << std::endl;
	frontEnd.bind(bindURI.str().c_str());

	backEnd.bind(SERVER_ADDR);

	sleep(1);

	/*
	 * Launch the worker threads
	 */
	std::vector<Server*> servers;
	int threadNum = Options::THREAD_NUM;
	if (threadNum == 0) {
		threadNum = std::thread::hardware_concurrency();
	}
	for (uint threadNum = 0; threadNum != Options::THREAD_NUM; threadNum++) {
		servers.push_back(new Server(merger, &context));
		servers[threadNum]->startThread(threadNum, -1, 15);
	}

	zmq::proxy(frontEnd, backEnd, NULL);

	return EXIT_SUCCESS;
}
