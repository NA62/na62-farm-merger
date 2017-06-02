/*
 * EobDimInfo.h
 *
 *  Created on: May 17, 2017
 *      Author: root
 */

#ifndef DIM_EOBDIMINFO_H_
#define DIM_EOBDIMINFO_H_

#include <cstring>
#include <map>
#include <vector>
#include <mutex>

#include <dic.hxx>
#include <structs/EOBPackets.h>
#include <options/Logging.h>

namespace na62 {
namespace merger {

class EobDimInfo : DimInfo {
public:
	EobDimInfo(std::string name): DimInfo(name.c_str(),-1) {}
	void infoHandler() {
	//put data into time-stamped indexed structure
		uint dataLength = getSize();

		if (dataLength < sizeof(EobDataHdr)) {
			LOG_ERROR("EOB Service " << getName() << " does not contain enough data: Found only "
					<< dataLength << " B but at least " << sizeof(EobDataHdr) << " B are needed to store the header ");
		} else {
			EobDataHdr* hdr = (EobDataHdr*) getData();
			if (hdr->length * 4 != dataLength) {
				LOG_ERROR("EOB Service " << getName() <<" does not contain the right number of bytes: The header says "
						<< hdr->length*4 << " but DIM stores " << dataLength);
			} else {
				char* buff = new char[dataLength];
				memcpy(buff, hdr, dataLength);
				LOG_INFO("Inserted data for " << getName() << " with TS " << (uint) hdr->eobTimestamp );
				std::lock_guard<std::mutex> lock(mapMutex_);
				eobDataByTS_[hdr->eobTimestamp] = (EobDataHdr*) buff;
			}
		}
	}
	EobDataHdr* getInfo(u_int32_t ts) {
		std::lock_guard<std::mutex> lock(mapMutex_);
		EobDataHdr* data = NULL;
		auto info = eobDataByTS_.find(ts);
		if (info != eobDataByTS_.end()) {
			data = info->second;
		}
		return data;
	}
	void clearInfo(u_int32_t ts) {
		// only remove from map, don't touch the data
		std::lock_guard<std::mutex> lock(mapMutex_);
		eobDataByTS_.erase(ts);
	}
private:
	std::map<u_int32_t, EobDataHdr*> eobDataByTS_;
	std::mutex mapMutex_;
};
}
}


#endif /* DIM_EOBDIMINFO_H_ */
