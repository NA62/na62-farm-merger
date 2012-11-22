/*
 * MonitorConnector.cpp
 *
 *  Created on: Nov 18, 2011
 *      Author: kunzejo
 */

#include "../options/Options.h"
#include "../messages/MessageHandler.h"
#include "MonitorConnector.h"

using namespace boost::interprocess;

namespace na62 {
namespace merger {

MonitorConnector::MonitorConnector(Merger& merger) :
		merger_(merger), currentState(OFF), timer_(monitoringService){

	mycout << "Started monitor connector" << std::endl;
	connectToDIMInterface();

	boost::thread(boost::bind(&MonitorConnector::thread, this));
}

void MonitorConnector::thread() {
	timer_.expires_from_now(boost::posix_time::milliseconds(Options::MONITOR_UPDATE_TIME));
	/*
	 * Add this PC to the table. As the Hostname-Column has a unique index we do not have to ckeck if the entry already exists
	 */
//	sqlConnector_.query("INSERT INTO `PC` (`Hostname`) VALUES ('" + std::string(hostName) + "')", false);
	timer_.async_wait(boost::bind(&MonitorConnector::handleUpdate, this));
	monitoringService.run();
}

MonitorConnector::~MonitorConnector() {
}

void MonitorConnector::updateState(STATE newState) {
	currentState = newState;

	if (!stateQueue_) {
		connectToDIMInterface();
		return;
	}
	try {
		if (!stateQueue_->try_send(&currentState, sizeof(STATE), 0)) {
			mycerr << "Unable to send message to DIM interface program!" << std::endl;
			connectToDIMInterface();
		}
	} catch (interprocess_exception &ex) {
		mycerr << "Unable to send message to DIM interface program: " << ex.what() << std::endl;
	}
}

void MonitorConnector::connectToDIMInterface() {
	try {
		stateQueue_.reset(new message_queue(open_only // only create
				, "state" // name
				));

		statisticsQueue_.reset(new message_queue(open_only // only create
				, "statistics" // name
				));

	} catch (interprocess_exception &ex) {
		stateQueue_.reset();
		statisticsQueue_.reset();
		mycerr << "Unable to connect to DIM interface program: " << ex.what() << std::endl;
	}
}

void MonitorConnector::handleUpdate() {
// Invoke this method every 'sleepTime' milliseconds
	timer_.expires_from_now(boost::posix_time::milliseconds(Options::MONITOR_UPDATE_TIME));
	timer_.async_wait(boost::bind(&MonitorConnector::handleUpdate, this));

	if (!stateQueue_) {
		connectToDIMInterface();
		return;
	}

	updateWatch_.reset();

	try {
		if (!stateQueue_->try_send(&currentState, sizeof(STATE), 0)) {
			mycout << "Unable to send message to DIM interface program!" << std::endl;
			connectToDIMInterface();
		}
	} catch (interprocess_exception &ex) {
		mycout << "Unable to send message queue DIM interface program: " << ex.what() << std::endl;
	}

	sendStatistics("BurstProgress", merger_.getProgressStats());
//	sendStatistics("BurstProgress", "2;3;");
}

void MonitorConnector::sendStatistics(std::string name, std::string values) {
	if (!statisticsQueue_ || name.empty() || values.empty()) {
		return;
	}

	std::string message = name + ":" + values;
	if (!statisticsQueue_->try_send(message.data(), message.length(), 0)) {
		mycerr << "Unable to send statistics to dim-service!" << std::endl;
	}
}

} /* namespace merger */
} /* namespace na62 */
