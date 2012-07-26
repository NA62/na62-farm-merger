/*
 * BrokenPacketReceivedError.h
 *
 *  Created on: Nov 15, 2011
 *      Author: kunzejo
 */

#ifndef UNDNOWNSOURCEIDFOUND_H_
#define UNDNOWNSOURCEIDFOUND_H_
#include <boost/lexical_cast.hpp>

#include <stdint.h>

#include "NA62Error.h"

namespace na62 {

class UnknownSourceIDFound: public na62::NA62Error {
public:
	UnknownSourceIDFound(uint8_t sourceID) :
			na62::NA62Error(
					"Unknown source ID: " + boost::lexical_cast<std::string>((int) sourceID)
							+ "\n Check the corresponding field in the Options file!") {
	}
};

} //namespace na62
#endif /* UNDNOWNSOURCEIDFOUND_H_ */
