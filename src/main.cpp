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
#include <csignal>

#include <options/Logging.h>
#include <socket/ZMQHandler.h>
#include <monitoring/IPCHandler.h>

#include "dim/MonitorConnector.h"
#include "merger/Merger.hpp"
#include "options/MyOptions.h"
#include "utils/Shutdown.h"

using namespace na62::merger;
using namespace na62;

class MergerShutdown: public Shutdown {
private:
	void myHandleStop(const boost::system::error_code& error, int signal_number) {
		LOG_INFO("Starting Safe Shutdown");
	}
};

int main(int argc, char* argv[]) {
	/*
	 * Read program parameters
	 */
	LOG_INFO("Initializing Options");
	MyOptions::Load(argc, argv);

	//Opening the socket
	zmq::context_t context(1);
	zmq::socket_t frontEnd(context, ZMQ_PULL);

	std::stringstream bindURI;
	try {
		bindURI << "tcp://*:" << Options::GetInt(OPTION_LISTEN_PORT);
    } catch(boost::bad_any_cast &e) {
    	LOG_ERROR("Cannot fetch the option " << e.what());
	}

	LOG_INFO("Opening ZMQ socket " << bindURI.str());
	try {
		frontEnd.bind(bindURI.str().c_str());
	} catch(zmq::error_t& e) {
		LOG_ERROR("Unable to bind the ZMQ socket: " << e.what());
		return EXIT_FAILURE;
	}

	int highWaterMark = 10000000;
	frontEnd.setsockopt(ZMQ_SNDHWM, &highWaterMark, sizeof(highWaterMark));

	/*
	 * Signals
	 */
	std::cout<<"Initializing safe ShutDown"<<std::endl;
	MergerShutdown safeShutdown;
	safeShutdown.init();

	na62::ZMQHandler::Initialize(1);

	Merger merger; //Aexecutable
	merger.startThread("Merger");

	LOG_INFO("Starting Monitoring Services");
	MonitorConnector monitor(merger); //Aexecutable
	monitor.startThread("MonitorConnector");
	monitor.updateState(na62::STATE::RUNNING);

    while (true) {
		zmq::message_t* eventMessage = new zmq::message_t();
		try {
			frontEnd.recv(eventMessage);
			merger.push(eventMessage);
		} catch(zmq::error_t& ex) {
			LOG_INFO("Interrupt received, proceeding…");
		}
        if (safeShutdown.isSignalReceived()) {
        	LOG_INFO("Interrupt received, killing server…");
            break;
        }
	}
    LOG_INFO("Closing ZMQ socket");
    frontEnd.close();
    LOG_INFO("Shutdown monitor Connector Thread");
    monitor.shutDown();

    LOG_INFO("Shutdown Merger Thread");
    merger.shutdown();

    /*
	 * Join threads
	 */
    LOG_INFO("Joining all threads");
    AExecutable::JoinAll();
    LOG_INFO("Returning Successful shutdown");
	return EXIT_SUCCESS;
}
