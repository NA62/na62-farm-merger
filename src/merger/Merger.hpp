/*
 * Merger.h
 *
 *  Created on: Jul 6, 2012
 *      Author: root
 */

#ifndef MERGER_H_
#define MERGER_H_

#include <stdint.h>
#include <iostream>
#include <boost/thread.hpp>

#include "../sockets/request.hpp"

namespace na62 {
namespace merger {

//class ThreadsafeMap;
class Merger {
public:
	void addPacket(EVENT& event);

	void print() {
		std::map<uint32_t, std::map<uint32_t, EVENT> >::const_iterator itr;
		for (itr = eventsByIDByBurst.begin(); itr != eventsByIDByBurst.end(); ++itr) {
			std::map<uint32_t, EVENT>::const_iterator itr2;
			for (itr2 = (*itr).second.begin(); itr2 != (*itr).second.end(); ++itr2) {
				std::cout << "Burst: " << (*itr).first << " EventKey: " << (*itr2).first << " EventNum: " << (*itr2).second.hdr->eventNum
						<< " lengt: " << (*itr2).second.hdr->length * 4 << std::endl;
			}
		}
	}

private:
	void startBurstControlThread(uint32_t& burstID);
	void saveBurst(std::map<uint32_t, EVENT>& eventByID, uint32_t& burstID);
	std::string generateFileName(uint32_t& burstID);
	void handle_newBurst(uint32_t newBurstID);
	void handle_burstFinished(uint32_t finishedBurstID);

	std::map<uint32_t, std::map<uint32_t, EVENT> > eventsByIDByBurst;

	boost::mutex newBurstMutex;
	boost::mutex eventMutex;
};

} /* namespace merger */
} /* namespace na62 */
#endif /* MERGER_H_ */
