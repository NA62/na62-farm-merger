/*
 * main.cpp
 *
 *  Created on: 21.12.2010
 *      Author: kunzej
 */

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "sockets/Receiver.hpp"
#include "options/Options.h"
#include "utils/Utils.h"

#include "dim/MonitorConnector.h"
#include "dim/CommandConnector.h"

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

	try {
		na62::merger::Receiver receiver(merger, Options::LISTEN_IP, Options::LISTEN_PORT, boost::thread::hardware_concurrency());

// Run the server until stopped.
		receiver.run();
	} catch (std::exception& e) {
		std::cerr << "exception: " << e.what() << "\n";
	}

	return EXIT_SUCCESS;
}
