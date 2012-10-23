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
#include "sockets/request.hpp"

#include "MonitorDimServer.h"

using namespace na62::merger;

int main(int argc, char* argv[]) {
	char hostName[1024];
	hostName[1023] = '\0';
	if (gethostname(hostName, 1023)) {
		std::cerr << "Unable to get host name! Refusing to start.";
		exit(1);
	}

	/*
	 * Read program parameters
	 */
	std::cout << "Initializing Options" << std::endl;
	Options::Initialize(argc, argv);

	Merger merger;
	MonitorDimServer(merger, std::string(hostName));

	try {
		na62::merger::Receiver receiver(merger, Options::LISTEN_IP, Options::LISTEN_PORT, boost::thread::hardware_concurrency());

// Run the server until stopped.
		receiver.run();
	} catch (std::exception& e) {
		std::cerr << "exception: " << e.what() << "\n";
	}

	return EXIT_SUCCESS;
}
