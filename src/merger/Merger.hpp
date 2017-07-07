/*
 * Merger.h
 *
 *  Created on: Jul 6, 2012
 *      Author: Jonas Kunze (kunze.jonas@gmail.com)
 */

#ifndef MERGER_H_
#define MERGER_H_

#include <boost/noncopyable.hpp>
#include <utils/AExecutable.h>

#include <atomic>
#include <mutex>
#include <stddef.h>
#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <structs/Event.h>
#include <zmq.hpp>
#include <tbb/tbb.h>

#include "../dim/EobCollector.h"
#include "../merger/ThreadPool.h"

namespace na62 {
namespace merger {

class Merger: private boost::noncopyable, public AExecutable   {
public:
	Merger();
	~Merger();
	void push(zmq::message_t* event);

	std::string getProgressStats() {
//		std::stringstream stream;
//		for (auto pair : eventsByBurstByID_) {
//			stream << pair.first << ":" << pair.second.size() << ";";
//		}
//		return stream.str();
		return burstWritePool_.getProgressStats();
	}



	uint getNumberOfEventsInLastBurst(){
		//return eventsInLastBurst_;
		return burstWritePool_.getNumberOfEventsInLastBurst();
	}

	std::mutex& getMutex(uint32_t burstID) {
		return *(mutexbyBurstID_[burstID]);
	}


	std::map<uint32_t, zmq::message_t*>& accessTo(uint32_t burstID) {
		return eventsByBurstByID_[burstID];
	}
	void erase(uint32_t burstID) {
		LOG_INFO("Deleting data of burst: " << burstID);
		eventsByBurstByID_.erase(burstID);
	}

	na62::merger::EobCollector& getEOBCollector() {
		return  eobCollector_;
	}

	std::string getStorageDir() const {
		return storageDir_;
	}



private:
	void thread();
	void startBurstControlThread(uint32_t& burstID);
	void saveBurst(std::map<uint32_t, zmq::message_t*>& eventByID, uint32_t& runNumber, uint32_t& burstID, uint32_t& sobTime);
	std::string generateFileName(uint32_t sob, uint32_t runNumber, uint32_t burstID, uint32_t duplicate);
	void handle_burstFinished(uint32_t finishedBurstID);

	na62::merger::EobCollector eobCollector_;
	//Burst eventID ZMQ message
	std::map<uint32_t, std::map<uint32_t, zmq::message_t*> > eventsByBurstByID_;
	std::map<uint32_t, std::mutex*> mutexbyBurstID_;


	uint eventsInLastBurst_;

	std::map<uint32_t, dim::BurstTimeInfo> burstInfos;

	tbb::concurrent_queue<zmq::message_t*> events_queue_;
	ThreadPool burstWritePool_;

	const std::string storageDir_;
	uint32_t lastSeenRunNumber_;



};

} /* namespace merger */
} /* namespace na62 */
#endif /* MERGER_H_ */
