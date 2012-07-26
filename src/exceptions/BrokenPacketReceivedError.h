/*
 * BrokenPacketReceivedError.h
 *
 *  Created on: Nov 15, 2011
 *      Author: kunzejo
 */

#ifndef BROKENPACKETRECEIVEDERROR_H_
#define BROKENPACKETRECEIVEDERROR_H_

#include "NA62Error.h"

namespace na62 {

class BrokenPacketReceivedError: public na62::NA62Error {
public:
	BrokenPacketReceivedError(const std::string& message) :
			na62::NA62Error(message) {
	}
};

} //namespace na62
#endif /* BROKENPACKETRECEIVEDERROR_H_ */
