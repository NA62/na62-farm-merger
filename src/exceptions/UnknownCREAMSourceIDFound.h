/*
 * BrokenPacketReceivedError.h
 *
 *  Created on: Nov 15, 2011
 *      Author: kunzejo
 */

#ifndef UNDNOWNCREAMSOURCEIDFOUND_H_
#define UNDNOWNCREAMSOURCEIDFOUND_H_
#include <boost/lexical_cast.hpp>
#include <stdint.h>
#include "NA62Error.h"

namespace na62 {
namespace merger {

class UnknownCREAMSourceIDFound: public NA62Error {
public:
	UnknownCREAMSourceIDFound(uint8_t crateID, uint8_t creamNum) :
			NA62Error(
					"Unknown CREAM source ID: CREAM " + boost::lexical_cast<std::string>((int) creamNum) + " in crate "
							+ boost::lexical_cast<std::string>((int) crateID) + "\n Check the corresponding field in the Options file!") {
	}
};

} //namespace merger
} //namespace na62
#endif /* UNDNOWNCREAMSOURCEIDFOUND_H_ */
