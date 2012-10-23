/*
 * MessageHandler.h
 *
 *  Created on: Nov 14, 2011
 *      Author: kunzejo
 */

#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include <iostream>
#include <boost/thread.hpp>
#include <stdio.h>
#include <streambuf>

namespace na62 {
namespace merger {

class MessageHandler: public std::streambuf {
public:
	MessageHandler(FILE * stream, bool ignoreVerbose = false) :
			stream_(stream), ignoreVerbose_(ignoreVerbose) {
	}
protected:
	/* central output function
	 * - print characters in uppercase mode
	 */
	virtual int overflow(int c);
private:
	FILE* stream_;
	bool ignoreVerbose_;
};

extern std::ostream mycout;
extern std::ostream mycerr;

} /* namespace merger */
} /* namespace na62 */
#endif /* MESSAGEHANDLER_H_ */
