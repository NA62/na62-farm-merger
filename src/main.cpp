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

using namespace na62;

int main(int argc, char* argv[]) {
	/*
	 * Read program parameters
	 */
	std::cout << "Initializing Options" << std::endl;
	Options::Initialize(argc, argv);

	try {
		na62::merger::Receiver s(Options::LISTEN_IP, Options::LISTEN_PORT, boost::thread::hardware_concurrency());
//		na62::merger::Receiver s(Options::LISTEN_IP, Options::LISTEN_PORT, 1);

// Run the server until stopped.
		s.run();
	} catch (std::exception& e) {
		std::cerr << "exception: " << e.what() << "\n";
	}

	return EXIT_SUCCESS;
}
