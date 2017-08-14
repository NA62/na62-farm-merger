/*
 * MonitorConnector.h
 *
 *  Created on: Nov 18, 2011
 *      Author: kunzejo
 */

#ifndef MONITORCONNECTOR_H_
#define MONITORCONNECTOR_H_

#include <stdlib.h>
#include <stdint.h>
#include <stack>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <map>
#include <boost/shared_ptr.hpp>

#include <monitoring/IPCHandler.h>
#include <utils/Stopwatch.h>
#include <utils/AExecutable.h>

#include "../merger/Merger.hpp"


namespace na62 {
namespace merger {

class MonitorConnector: public AExecutable  {
public:
	MonitorConnector(Merger& merger);
	virtual ~MonitorConnector();

	void updateState(na62::STATE newState);
	void shutDown();

private:
	virtual void thread();
	void handleUpdate();
	void onInterruption();
	void sendStatistics(std::string name, std::string values);


	Merger& merger_;

	boost::asio::io_service monitoringService;

	na62::STATE currentState;

	Stopwatch updateWatch_;

	boost::asio::deadline_timer timer_;

};

} /* namespace merger */
} /* namespace na62 */
#endif /* MONITORCONNECTOR_H_ */
