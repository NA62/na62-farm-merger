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
#include <vector>
#include <thread>

#include <socket/ZMQHandler.h>
#include <monitoring/IPCHandler.h>

#include "dim/CommandConnector.h"
#include "dim/MonitorConnector.h"
#include "merger/Merger.hpp"
#include "options/Options.h"

using namespace na62::merger;
using namespace na62;

int main(int argc, char* argv[]) {
	/*
	 * Read program parameters
	 */
	std::cout << "Initializing Options" << std::endl;
	Options::Initialize(argc, argv);

	na62::ZMQHandler::Initialize(1);

	Merger merger;
	MonitorConnector monitor(merger);

	CommandConnector commands(merger);
	commands.startThread("CommandConnector");

	zmq::context_t context(1);
	zmq::socket_t frontEnd(context, ZMQ_PULL);

	std::stringstream bindURI;
	bindURI << "tcp://*:" << Options::LISTEN_PORT;

	std::cout << "Opening ZMQ socket " << bindURI.str() << std::endl;
	frontEnd.bind(bindURI.str().c_str());

	int highWaterMark = 10000000;
	frontEnd.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof(highWaterMark));

	monitor.updateState(na62::STATE::RUNNING);


	while (true) {
		//  Wait for next request from client
		zmq::message_t* eventMessage = new zmq::message_t();
		frontEnd.recv(eventMessage);
		EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

		if (eventMessage->size() == event->length * 4) {
			merger.addPacket(eventMessage);
		} else {
			std::cerr << "Received " << eventMessage->size() << " Bytes with an event of length " << (event->length * 4) << std::endl;
		}
	}

	return EXIT_SUCCESS;
}
