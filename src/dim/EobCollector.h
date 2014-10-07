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

namespace na62 {
struct EVENT_HDR;
} /* namespace na62 */

class DimInfo;

namespace na62 {
namespace dim {

struct EobDataHdr {
	u_int16_t length; // number of 32-bit words including this header
	u_int8_t blockID;
	u_int8_t detectorID;

	u_int32_t eobTimestamp;
}__attribute__ ((__packed__));

class EobCollector {
public:
	EobCollector();
	virtual ~EobCollector();

	void run();

	/**
	 * Writes the EOB data from dim to the end of every detector block in the event.
	 * event will be deleted in this method if any EOB data was found. You should therefore exchange event with the returned pointer.
	 */
	EVENT_HDR* addEobDataToEvent(EVENT_HDR* event);
private:
	DimListener dimListener_;
	std::map<uint, std::map<uint8_t, std::vector<EobDataHdr*>>>eobDataByBurstIDBySourceID;

	/**
	 * Returns the data stored at the service identified by the given serviceName.
	 *
	 * If the data is corrupt (e.g. wrong length) nullptr will be returned
	 */
	EobDataHdr* getData(DimInfo* dimInfo);

	std::mutex eobCallbackMutex_;

	std::map <std::string, DimInfo*> eobInfoByName_;
};

}
}
#endif /* EOBCOLLECTOR_H_ */
