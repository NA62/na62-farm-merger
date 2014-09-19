/*
 * CommandConnector.cpp
 *
 *  Created on: Jul 25, 2012
 *      Author: root
 */

#include "CommandConnector.h"

#include <boost/lexical_cast.hpp>
#include <monitoring/IPCHandler.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "../messages/MessageHandler.h"

namespace na62 {
namespace merger {

CommandConnector::CommandConnector(Merger& merger) :
		merger_(merger) {
}

CommandConnector::~CommandConnector() {
}

void CommandConnector::thread() {
	std::string message;
	while (true) {
		message = IPCHandler::getNextCommand();

		std::cout << "Received command: " << message << std::endl;
		std::transform(message.begin(), message.end(), message.begin(), ::tolower);

		std::vector<std::string> strings;
		boost::split(strings, message, boost::is_any_of(":"));
		if (strings.size() != 2) {
			mycerr << "Unknown command: " << message << std::endl;
		} else {
			std::string command = strings[0];
			if (command == "updaterunnumber") {
				uint32_t runNumber = boost::lexical_cast<int>(strings[1]);
				mycout << "Updating runNumber to " << runNumber << std::endl;
				merger_.updateRunNumber(runNumber);
			}  else if (message.size() > 14 && message.substr(0, 14) == "sob_timestamp:") {
				std::string timeStampString = message.substr(14, message.size() - 14);
				uint32_t timestamp = atol(timeStampString.data());
				merger_.SetSOBtimestamp_(timestamp);
			}
		}
	}
}

} /* namespace merger */
} /* namespace na62 */
