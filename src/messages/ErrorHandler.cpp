/*
 * ErrorHandler.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: kunzejo
 */

#include "ErrorHandler.h"

namespace na62 {

void ErrorHandler::Write(const std::string& message) {
	MessageHandler::Write(message);
}

} /* namespace na62 */
