/*
 * WanringHandler.h
 *
 *  Created on: Nov 14, 2011
 *      Author: kunzejo
 */

#ifndef WANRINGHANDLER_H_
#define WANRINGHANDLER_H_

#include "MessageHandler.h"

namespace na62 {

class WarningHandler {
public:
	static void Write(const std::string& message);
};

} /* namespace na62 */
#endif /* WANRINGHANDLER_H_ */
