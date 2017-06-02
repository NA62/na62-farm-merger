/*
 * BurstInfo.h
 *
 *  Created on: May 22, 2017
 *      Author: root
 */

#ifndef DIM_BURSTINFO_H_
#define DIM_BURSTINFO_H_

#include<atomic>

namespace na62 {
namespace merger {

struct BurstTimeInfo{
	int32_t burstID;
	int32_t sobTime;
	int32_t eobTime;
	int32_t runNumber;
};

class BurstDimInfo : DimInfo {

public:
	BurstDimInfo(): DimInfo("RunControl/BurstTimeStruct",-1) {lastRun_ = 0;}
	void infoHandler() {

	//put data into time-stamped indexed structure
		uint dataLength = getSize();

		if (dataLength != sizeof(BurstTimeInfo)) {
			LOG_ERROR("DIM Service " << getName() << " does not contain enough data: Found only "
					<< dataLength << " B but at least " << sizeof(BurstTimeInfo) << " B are needed to store the burst time information ");
		} else {
			BurstTimeInfo* hdr = (BurstTimeInfo*) getData();

			if(hdr->runNumber >= lastRun_+2) {
				std::lock_guard<std::mutex> lock(mapMutex_);
				sobDataByBurstID_.clear();
				eobDataByBurstID_.clear();
				lastRun_ = hdr->runNumber;
			}
			if(hdr->eobTime ==0) {
				std::lock_guard<std::mutex> lock(mapMutex_);
				sobDataByBurstID_.insert(std::make_pair(hdr->sobTime, *hdr));
				return;
			}
			else {
				std::lock_guard<std::mutex> lock(mapMutex_);
				eobDataByBurstID_.insert(std::make_pair(hdr->eobTime, *hdr));
			}
		}
	}

	BurstTimeInfo getInfoSOB(u_int32_t burstID) {
		std::lock_guard<std::mutex> lock(mapMutex_);
		return sobDataByBurstID_.at(burstID);
	}

	BurstTimeInfo getInfoEOB(u_int32_t burstID) {
		std::lock_guard<std::mutex> lock(mapMutex_);
		return eobDataByBurstID_.at(burstID);
	}

private:
	std::map<u_int32_t, BurstTimeInfo> sobDataByBurstID_;
	std::map<u_int32_t, BurstTimeInfo> eobDataByBurstID_;
	std::mutex mapMutex_;
	std::atomic<int32_t> lastRun_;
};

}
}



#endif /* DIM_BURSTINFO_H_ */
