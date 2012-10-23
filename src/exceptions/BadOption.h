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
namespace merger {
class BadOption: public NA62Error {
public:
	BadOption(std::string option, std::string value) :
			NA62Error("Bad option '"+option+"': "+value+"\n\n Try --help") {
	}
};
} //namespace merger
} //namespace na62
#endif /* BADOPTION_H_ */
