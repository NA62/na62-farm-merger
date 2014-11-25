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
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>

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

		LOG_INFO << "Received command: " << message;
		std::transform(message.begin(), message.end(), message.begin(), ::tolower);

		std::vector<std::string> strings;
		boost::split(strings, message, boost::is_any_of(":"));
		if (strings.size() != 2) {
			LOG_ERROR << "Unknown command: " << message;
		} else {
			std::string command = strings[0];
			if (command == "updaterunnumber") {
				uint32_t runNumber = boost::lexical_cast<int>(strings[1]);
				LOG_INFO << "Updating runNumber to " << runNumber;
				merger_.updateRunNumber(runNumber);
			}  else if (message.size() > 14 && message.substr(0, 14) == "sob_timestamp:") {
				std::string timeStampString = message.substr(14, message.size() - 14);
				uint32_t timestamp = atol(timeStampString.data());
				merger_.SetSOBtimestamp(timestamp);
			}
		}
	}
}

} /* namespace merger */
} /* namespace na62 */
