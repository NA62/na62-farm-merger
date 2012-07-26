/*
 * BrokenPacketReceivedError.h
 *
 *  Created on: Nov 15, 2011
 *      Author: kunzejo
 */

#ifndef BADOPTION_H_
#define BADOPTION_H_
#include "NA62Error.h"

namespace na62 {

class BadOption: public na62::NA62Error {
public:
	BadOption(std::string option, std::string value) :
			na62::NA62Error("Bad option '"+option+"': "+value+"\n\n Try --help") {
	}
};

} //namespace na62
#endif /* BADOPTION_H_ */
