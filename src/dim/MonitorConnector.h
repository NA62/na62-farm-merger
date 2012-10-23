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
#include <boost/interprocess/ipc/message_queue.hpp>
#include <map>
#include <boost/shared_ptr.hpp>

#include "../utils/Stopwatch.h"
#include "../merger/Merger.hpp"


namespace na62 {
namespace merger {


enum STATE {
	// 0=IDLE; 1=INITIALIZED; 2=RUNNING; Other=ERROR
	OFF,
	INITIALIZED,
	RUNNING,
	ERROR
};

class MonitorConnector {
public:
	MonitorConnector(Merger& merger);
	virtual ~MonitorConnector();

	void updateState(STATE newState);

private:
	virtual void thread();
	void handleUpdate();
	void sendStatistics(std::string name, std::string values);

	void connectToDIMInterface();

	Merger& merger_;

	typedef boost::shared_ptr<boost::interprocess::message_queue> message_queue_ptr;

	boost::asio::io_service monitoringService;

	STATE currentState;
	message_queue_ptr stateQueue_;
	message_queue_ptr statisticsQueue_;

	Stopwatch updateWatch_;

	boost::asio::deadline_timer timer_;

};

} /* namespace merger */
} /* namespace na62 */
#endif /* MONITORCONNECTOR_H_ */
