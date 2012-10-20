/*
 * MessageHandler.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: kunzejo
 */

#include "../options/Options.h"
#include "MessageHandler.h"

namespace na62 {

Stopwatch MessageHandler::lastMessageTimer;
boost::mutex MessageHandler::echoMutex;
//SQLConnector* MessageHandler::sqlConnector = NULL;

void MessageHandler::Write(const std::string& message) {
	Write(message, "Message");
}

void MessageHandler::Write(const std::string& message, const std::string& tableName) {
	boost::lock_guard<boost::mutex> lock(echoMutex); // Will lock until return
	if (lastMessageTimer.getRealTime() > 1) {
		std::cout << message << std::endl;
		lastMessageTimer.restart();
	}

//	if (Options::Instance() != NULL) {
//		/*
//		 * Search for ' and replace it by \'
//		 */
//		size_t pos = 0;
//		std::string sqlSafe(message);
//		while ((pos = sqlSafe.find("'", pos)) != std::string::npos) {
//			sqlSafe.replace(pos, 2, (char*) "\'");
//			pos += 2;
//		}
//
//		if (sqlConnector == NULL) {
//			sqlConnector = new SQLConnector();
//		}
//		sqlConnector->query("INSERT INTO  `na62-monitoring`.`" + tableName + "` (`Text`)	VALUES ( '" + sqlSafe + "')");
//	}
}

} /* namespace na62 */
