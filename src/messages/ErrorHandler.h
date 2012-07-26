/*
 * ErrorHandler.h
 *
 *  Created on: Nov 14, 2011
 *      Author: kunzejo
 */

#ifndef ERRORHANDLER_H_
#define ERRORHANDLER_H_

#include "MessageHandler.h"

namespace na62 {

class ErrorHandler {
public:
	static void Write(const std::string& message);
};

} /* namespace na62 */
#endif /* ERRORHANDLER_H_ */
