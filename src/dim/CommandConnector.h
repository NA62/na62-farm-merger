/*
 * CommandConnector.h
 *
 *  Created on: Jul 25, 2012
 *      Author: root
 */

#ifndef COMMANDCONNECTOR_H_
#define COMMANDCONNECTOR_H_

#include <boost/noncopyable.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "../utils/AExecutable.h"
#include "../merger/Merger.hpp"

namespace na62 {
namespace merger {
class CommandConnector: private boost::noncopyable, public AExecutable {
public:
	CommandConnector(Merger& merger);
	virtual ~CommandConnector();
	void run();
private:
	void thread();

	Merger& merger_;

	typedef boost::shared_ptr<boost::interprocess::message_queue> message_queue_ptr;
	message_queue_ptr commandQueue_;
};

} /* namespace merger */
} /* namespace na62 */
#endif /* COMMANDCONNECTOR_H_ */