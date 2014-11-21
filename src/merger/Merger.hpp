/*
 * Merger.h
 *
 *  Created on: Jul 6, 2012
 *      Author: root
 */

#ifndef MERGER_H_
#define MERGER_H_

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
//				std::cout << "Burst: " << pair1.first << " EventKey: " << pair2.first << " EventNum: " << pair2.second->eventNum << " lengt: "
//						<< pair2.second->length * 4 << std::endl;
//			}
//		}
//	}

	std::string getProgressStats() {
		std::stringstream stream;
		for (auto pair : eventsByBurstByID) {
			stream << pair.first << ";" << pair.second.size() << ";";
		}
		return stream.str();
	}

	void updateRunNumber(uint32_t newRunNumber) {
		currentRunNumber_ = newRunNumber;
	}

	void SetSOBtimestamp(uint32_t SOBtimestamp) {
		nextBurstSOBtimestamp_ = SOBtimestamp;
	}

private:
	void startBurstControlThread(uint32_t& burstID);
	void saveBurst(std::map<uint32_t, zmq::message_t*>& eventByID, uint32_t& burstID);
	void writeBKMFile(std::string dataFilePath, std::string fileName, size_t fileLength);
	std::string generateFileName(uint32_t sob, uint32_t runNumber, uint32_t burstID, uint32_t duplicate);
	void handle_newBurst(uint32_t newBurstID);
	void handle_burstFinished(uint32_t finishedBurstID);

	dim::EobCollector eobCollector_;

	std::map<uint32_t, std::map<uint32_t, zmq::message_t*> > eventsByBurstByID;
	std::map<uint32_t, uint32_t> runNumberByBurst;
	std::map<uint32_t, uint32_t> SOBtimestampByBurst;
	uint32_t currentRunNumber_;
	uint32_t nextBurstSOBtimestamp_;

	std::mutex newBurstMutex;
	std::mutex eventMutex;

	uint ownerID_;
	uint groupID_;

	const std::string storageDir_;
};

} /* namespace merger */
} /* namespace na62 */
#endif /* MERGER_H_ */
