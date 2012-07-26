/*
 * MessageHandler.h
 *
 *  Created on: Nov 14, 2011
 *      Author: kunzejo
 */

#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include <iostream>
#include <boost/thread.hpp>

#include "../utils/Stopwatch.h"

namespace na62 {

// forward Declaration
class SQLConnector;

class MessageHandler {
public:
	static void Write(const std::string& message);
	static void Write(const std::string& message, const std::string& tableName);
private:
	static Stopwatch lastMessageTimer;
	static boost::mutex echoMutex;
	static SQLConnector* sqlConnector;
};

} /* namespace na62 */
#endif /* MESSAGEHANDLER_H_ */
