/*
 * EobCollector.h
 *
 *  Created on: Sep 23, 2014
 *      Author: Jonas Kunze <kunze.jonas@gmail.com>
 */

#ifndef EOBCOLLECTOR_H_
#define EOBCOLLECTOR_H_

#include <dim/DimListener.h>
#include <sys/types.h>
#include <map>
#include <vector>
#include <mutex>
#include <zmq.hpp>
#include "EobDimInfo.h"
#include "BurstInfo.h"

namespace na62 {
struct EVENT_HDR;
} /* namespace na62 */

//class DimInfo;

namespace na62 {
namespace merger {

class EobCollector {
public:
	EobCollector();
	virtual ~EobCollector();

	void run();

	/**
	 * Writes the EOB data from dim to the end of every detector block in the event.
	 * event will be deleted in this method if any EOB data was found. You should therefore exchange event with the returned pointer.
	 */
	//zmq::message_t* addEobDataToEvent(zmq::message_t* event);
	char* addEobDataToEvent(zmq::message_t* event);
	BurstTimeInfo getBurstInfoSOB(uint32_t burstID) {
		return burstInfo_.getInfoSOB(burstID);
	}

	BurstTimeInfo getBurstInfoEOB(uint32_t burstID) {
		return burstInfo_.getInfoEOB(burstID);
	}

private:

	/**
	 * Returns the data stored at the service identified by the given serviceName.
	 *
	 * If the data is corrupt (e.g. wrong length) nullptr will be returned
	 */
	EobDataHdr* getData(DimInfo* dimInfo);

	std::mutex eobCallbackMutex_;

	std::map <std::string, EobDimInfo*> eobInfoByName_;
	BurstDimInfo burstInfo_;
};

}
}
#endif /* EOBCOLLECTOR_H_ */
