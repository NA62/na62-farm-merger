/*
 * Merger.h
 *
 *  Created on: Jul 6, 2012
 *      Author: Jonas Kunze (kunze.jonas@gmail.com)
 */

#ifndef MERGER_H_
#define MERGER_H_

#include <atomic>
#include <mutex>
#include <stddef.h>
#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <structs/Event.h>
#include <zmq.hpp>

#include "../dim/EobCollector.h"

namespace na62 {
namespace merger {

//class ThreadsafeMap;
class Merger {
public:
	Merger();
	void addPacket(zmq::message_t* event);

//	void print() {
//		for (auto pair1 : eventsByIDByBurst) {
//			std::map<uint32_t, EVENT_HDR*>::const_iterator itr2;
//			for (auto pair2 : pair1.second) {
//				LOG_INFO("Burst: " << pair1.first << " EventKey: " << pair2.first << " EventNum: " << pair2.second->eventNum << " lengt: "
//						<< pair2.second->length * 4);
//			}
//		}
//	}

	std::string getProgressStats() {
		std::stringstream stream;
		for (auto pair : eventsByBurstByID) {
			stream << pair.first << ":" << pair.second.size() << ";";
		}
		return stream.str();
	}

	uint getNumberOfEventsInLastBurst(){
		return eventsInLastBurst_;
	}

	//void updateRunNumber(uint32_t newRunNumber) {
	//	currentRunNumber_ = newRunNumber;
	//}

	//void SetSOBtimestamp(uint32_t SOBtimestamp) {
	//	nextBurstSOBtimestamp_ = SOBtimestamp;
	//}

private:
	void startBurstControlThread(uint32_t& burstID);
	void saveBurst(std::map<uint32_t, zmq::message_t*>& eventByID, uint32_t& runNumber, uint32_t& burstID, uint32_t& sobTime);
	std::string generateFileName(uint32_t sob, uint32_t runNumber, uint32_t burstID, uint32_t duplicate);
	void handle_newBurst(uint32_t newBurstID);
	void handle_burstFinished(uint32_t finishedBurstID);

	dim::EobCollector eobCollector_;

	std::map<uint32_t, std::map<uint32_t, zmq::message_t*> > eventsByBurstByID;
	uint eventsInLastBurst_;

	std::map<uint32_t, dim::BurstTimeInfo> burstInfos;
	//std::map<uint32_t, uint32_t> runNumberByBurst;
	//std::map<uint32_t, uint32_t> SOBtimestampByBurst;
	//uint32_t nextBurstSOBtimestamp_;

	std::mutex newBurstMutex;
	std::mutex eventMutex;

	const std::string storageDir_;
	uint32_t lastSeenRunNumber_;

};

} /* namespace merger */
} /* namespace na62 */
#endif /* MERGER_H_ */
