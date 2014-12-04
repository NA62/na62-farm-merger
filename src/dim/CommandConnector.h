/*
 * CommandConnector.h
 *
 *  Created on: Jul 25, 2012
 *      Author: Jonas Kunze (kunze.jonas@gmail.com)
 */

#ifndef COMMANDCONNECTOR_H_
#define COMMANDCONNECTOR_H_

#include <boost/noncopyable.hpp>

#include <utils/AExecutable.h>
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
};

} /* namespace merger */
} /* namespace na62 */
#endif /* COMMANDCONNECTOR_H_ */
