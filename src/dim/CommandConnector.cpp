/*
 * CommandConnector.cpp
 *
 *  Created on: Jul 25, 2012
 *      Author: root
 */

#include <boost/asio.hpp>
#include "CommandConnector.h"

namespace na62 {
namespace merger {
using namespace boost::interprocess;

CommandConnector::CommandConnector(Merger& merger) :
		merger_(merger) {
	try {
		// Erase previous message queue
		message_queue::remove("command");
		// Open a message queue.
		commandQueue_.reset(new boost::interprocess::message_queue(create_only //only create
				, "command" //name
				, 1 // max message number
				, sizeof(int) //max message size
				));

		unsigned int priority;
		message_queue::size_type recvd_size;
	} catch (interprocess_exception &ex) {
		message_queue::remove("command");
		mycerr << "Unable to create command message queue: " << ex.what() << std::endl;
		boost::system::error_code noError;
	}
}

CommandConnector::~CommandConnector() {
	message_queue::remove("command");
}

void CommandConnector::thread() {
	unsigned int priority;
	message_queue::size_type recvd_size;

	std::string message;
	while (true) {
		message.resize(1024 * 64);
		boost::posix_time::time_duration timeoutDuration(boost::posix_time::seconds(2));

		boost::posix_time::ptime timeout = boost::posix_time::ptime(boost::posix_time::second_clock::universal_time() + timeoutDuration);

		/*
		 * Synchronious receive:
		 */
		commandQueue_->receive(&(message[0]), message.size(), recvd_size, priority);
		message.resize(recvd_size);
		mycout << "Received command: " << message << std::endl;

		std::vector<std::string> strings;
		boost::split(strings, message, boost::is_any_of(":"));
		if (strings.size() != 2) {
			mycerr << "Unknown command: " << message << std::endl;
		} else {
			std::string command = strings[0];
			if (command == "UpdateRunNumber") {
				uint32_t runNumber = boost::lexical_cast<int>(strings[1]);
				mycout << "Updating runNumber to " << runNumber << std::endl;
				merger_.updateRunNumber(runNumber);
			}
		}
	}
}

} /* namespace merger */
} /* namespace na62 */
