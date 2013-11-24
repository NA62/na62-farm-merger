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

#include "../sockets/Event.hpp"

namespace na62 {
namespace merger {

//class ThreadsafeMap;
class Merger {
public:
	Merger();
	void addPacket(EVENT* event);

	void print() {
		std::map<uint32_t, std::map<uint32_t, EVENT*> >::const_iterator itr;
		for (itr = eventsByIDByBurst.begin(); itr != eventsByIDByBurst.end(); ++itr) {
			std::map<uint32_t, EVENT*>::const_iterator itr2;
			for (itr2 = (*itr).second.begin(); itr2 != (*itr).second.end(); ++itr2) {
				std::cout << "Burst: " << (*itr).first << " EventKey: " << (*itr2).first << " EventNum: " << (*itr2).second->eventNum
						<< " lengt: " << (*itr2).second->length * 4 << std::endl;
			}
		}
	}

	std::string getProgressStats() {
		std::stringstream stream;
		std::map<uint32_t, std::map<uint32_t, EVENT*> >::const_iterator itr;
		for (itr = eventsByIDByBurst.begin(); itr != eventsByIDByBurst.end(); ++itr) {
			stream << (*itr).first << ";" << (*itr).second.size() << ";";
		}
		return stream.str();
	}

	void updateRunNumber(uint32_t newRunNumber) {
		currentRunNumber_ = newRunNumber;
	}

	void SetSOBtimestamp_(uint32_t SOBtimestamp) {
		nextBurstSOBtimestamp_ = SOBtimestamp;
	}


private:
	void startBurstControlThread(uint32_t& burstID);
	void saveBurst(std::map<uint32_t, EVENT*>& eventByID, uint32_t& burstID);
	void writeBKMFile(std::string dataFilePath, std::string fileName, size_t fileLength);
	std::string generateFileName(uint32_t runNumber, uint32_t burstID, uint32_t duplicate);
	void handle_newBurst(uint32_t newBurstID);
	void handle_burstFinished(uint32_t finishedBurstID);

	std::map<uint32_t, std::map<uint32_t, EVENT*> > eventsByIDByBurst;
	std::map<uint32_t, uint32_t> runNumberByBurst;
	std::map<uint32_t, uint32_t> SOBtimestampByBurst;
	uint32_t currentRunNumber_;
	uint32_t nextBurstSOBtimestamp_;

	boost::mutex newBurstMutex;
	boost::mutex eventMutex;
};

} /* namespace merger */
} /* namespace na62 */
#endif /* MERGER_H_ */
