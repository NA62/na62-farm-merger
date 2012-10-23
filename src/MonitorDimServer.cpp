/*
 * MonitorDimServer.cpp
 *
 *  Created on: Jun 20, 2012
 *      Author: kunzejo
 */

#include "options/Options.h"
#include "MonitorDimServer.h"

namespace na62 {
namespace merger {

MonitorDimServer::MonitorDimServer(Merger& merger, std::string hostName) :
		merger_(merger), hostName_(hostName), burstProgressService_(std::string(hostName + "/BurstProgress").data(), (char*) ""), timer_(
				updateService_) {

	timer_.expires_from_now(boost::posix_time::milliseconds(Options::DIM_UPDATE_TIME));
	timer_.async_wait(boost::bind(&MonitorDimServer::updateStatistics, this));
	boost::thread simulatorThread(boost::bind(&boost::asio::io_service::run, &updateService_));

	start(hostName_.data());
	std::cout << "Started dim Server" << std::endl;
}

MonitorDimServer::~MonitorDimServer() {
	stop();
}

void MonitorDimServer::updateStatistics() {
	timer_.expires_from_now(boost::posix_time::milliseconds(Options::DIM_UPDATE_TIME));
	timer_.async_wait(boost::bind(&MonitorDimServer::updateStatistics, this));

	std::cout << "Updating Statistics" << std::endl;
	burstProgressService_.updateService((char*) "");
}

} /* namespace merger */
} /* namespace na62 */
