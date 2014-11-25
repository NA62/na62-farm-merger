/*
 * MonitorConnector.cpp
 *
 *  Created on: Nov 18, 2011
 *      Author: kunzejo
 */
#include <glog/logging.h>

#include "../options/MyOptions.h"
#include "MonitorConnector.h"

namespace na62 {
namespace merger {

using namespace na62;

MonitorConnector::MonitorConnector(Merger& merger) :
		merger_(merger), currentState(OFF), timer_(monitoringService) {

	LOG_INFO << "Started monitor connector";

	boost::thread(boost::bind(&MonitorConnector::thread, this));
}

void MonitorConnector::thread() {
	timer_.expires_from_now(boost::posix_time::milliseconds(Options::GetInt(OPTION_DIM_UPDATE_TIME)));
	/*
	 * Add this PC to the table. As the Hostname-Column has a unique index we do not have to ckeck if the entry already exists
	 */
//	sqlConnector_.query("INSERT INTO `PC` (`Hostname`) VALUES ('" + std::string(hostName) + "')", false);
	timer_.async_wait(boost::bind(&MonitorConnector::handleUpdate, this));
	monitoringService.run();
}

MonitorConnector::~MonitorConnector() {
}

void MonitorConnector::updateState(na62::STATE newState) {
	currentState = newState;

	IPCHandler::updateState (currentState);
}

void MonitorConnector::handleUpdate() {
// Invoke this method every 'sleepTime' milliseconds
	timer_.expires_from_now(boost::posix_time::milliseconds(Options::GetInt(OPTION_DIM_UPDATE_TIME)));
	timer_.async_wait(boost::bind(&MonitorConnector::handleUpdate, this));

	updateWatch_.reset();

	IPCHandler::updateState (currentState);

	sendStatistics("BurstProgress", merger_.getProgressStats());
}

void MonitorConnector::sendStatistics(std::string name, std::string values) {
	IPCHandler::sendStatistics(name, values);
}

} /* namespace merger */
} /* namespace na62 */
