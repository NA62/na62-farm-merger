/*
 * NA62Error.cpp
 *
 *  Created on: Nov 15, 2011
 *      Author: kunzejo
 */

#include "NA62Error.h"

namespace na62 {

NA62Error::NA62Error(const std::string& message) :
		std::runtime_error(message) {
	na62::ErrorHandler::Write(message);
}

} /* namespace na62 */
