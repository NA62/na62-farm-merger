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

#include <options/Logging.h>
#include <socket/ZMQHandler.h>
#include <monitoring/IPCHandler.h>

#include "dim/MonitorConnector.h"
#include "merger/Merger.hpp"
#include "options/MyOptions.h"

using namespace na62::merger;
using namespace na62;

int main(int argc, char* argv[]) {
	/*
	 * Read program parameters
	 */
	LOG_INFO("Initializing Options");
	MyOptions::Load(argc, argv);

	na62::ZMQHandler::Initialize(1);

	Merger merger;
	merger.startThread("Merger");

	MonitorConnector monitor(merger);



	zmq::context_t context(1);
	zmq::socket_t frontEnd(context, ZMQ_PULL);

	std::stringstream bindURI;
	bindURI << "tcp://*:" << Options::GetInt(OPTION_LISTEN_PORT);

	LOG_INFO("Opening ZMQ socket " << bindURI.str());
	frontEnd.bind(bindURI.str().c_str());

	int highWaterMark = 10000000;
	frontEnd.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof(highWaterMark));

	monitor.updateState(na62::STATE::RUNNING);


	while (true) {
		//  Wait for next request from client
		zmq::message_t* eventMessage = new zmq::message_t();
		frontEnd.recv(eventMessage);
		merger.push(eventMessage);
	}
	/*
	 * Join PacketHandler and other threads
	 */
	AExecutable::JoinAll();
	return EXIT_SUCCESS;
}
