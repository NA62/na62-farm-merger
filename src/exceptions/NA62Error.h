/*
 * NA62Error.h
 *
 *  Created on: Nov 15, 2011
 *      Author: kunzejo
 */

#ifndef NA62ERROR_H_
#define NA62ERROR_H_

#include <stdexcept>

#include "../messages/ErrorHandler.h"

namespace na62 {

class NA62Error: public std::runtime_error {
public:
	NA62Error(const std::string& message);
};

} /* namespace na62 */
#endif /* NA62ERROR_H_ */
