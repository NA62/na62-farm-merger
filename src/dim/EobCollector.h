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

namespace na62 {
struct EVENT_HDR;
} /* namespace na62 */

namespace na62 {
namespace dim {

struct EobDataHdr {
	u_int16_t length; // number of bytes including this header
	u_int8_t blockID;
	u_int8_t detectorID;
	u_int32_t eobTimestamp;
}__attribute__ ((__packed__));

class EobCollector {
public:
	EobCollector();
	virtual ~EobCollector();

	void run();

	EVENT_HDR* addEobDataToEvent(EVENT_HDR* event);
private:
	DimListener dimListener_;
	std::map<uint, std::map<uint8_t, std::vector<EobDataHdr*>>> eobDataBySourceIDByBurstID;

	/**
	 * Returns the data stored at the service identified by the given serviceName.
	 *
	 * If the data is corrupt (e.g. wrong length) nullptr will be returned
	 */
	EobDataHdr* getDataHdr(char* serviceName);
};

}
}
#endif /* EOBCOLLECTOR_H_ */
