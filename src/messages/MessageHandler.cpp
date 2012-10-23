/*
 * MessageHandler.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: root
 */

#include "MessageHandler.h"
#include "../options/Options.h"
#include <streambuf>

namespace na62 {
namespace merger {

int MessageHandler::overflow(int c) {
	if (Options::VERBOSE == 0 || (!ignoreVerbose_ && Options::VERBOSE == 1)) {
		return EOF;
	}

	if (c != EOF) {
		// and write the character to the standard output
		if (putc(c, stream_) == EOF) {
			return EOF;
		}
	}
	return c;
}

MessageHandler mH(stdout);
MessageHandler eH(stderr, true);
// initialize output streams with the output buffers
std::ostream mycout(&mH);
std::ostream mycerr(&eH);

} /* namespace merger */
} /* namespace na62 */
