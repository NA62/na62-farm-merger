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
	BurstDimInfo(): DimInfo("RunControl/BurstTimeStruct",-1) {}
	void infoHandler() {

	//put data into time-stamped indexed structure
		uint dataLength = getSize();

		if (dataLength != sizeof(BurstTimeInfo)) {
			LOG_ERROR("DIM Service " << getName() << " does not contain enough data: Found only "
					<< dataLength << " B but at least " << sizeof(BurstTimeInfo) << " B are needed to store the burst time information ");
		} else {
			BurstTimeInfo* hdr = (BurstTimeInfo*) getData();

			LOG_INFO("Handling RunControl/BurstTimeStruct: runnumber=" << hdr->runNumber << " sobTime="<< hdr->sobTime << " eobTime=" << hdr->eobTime );

			//if (hdr->runNumber >= lastRun_+2) {
				//std::lock_guard<std::mutex> lock(mapMutex_);
				//sobDataByBurstID_.clear();
				//eobDataByBurstID_.clear();
				//lastRun_ = hdr->runNumber;
			//}



//			if (hdr->sobTime == 0) {
//				LOG_ERROR("Handled a 0 SOB");
//			} else {
//					std::lock_guard<std::mutex> lock(mapMutex_);
//					sobDataByBurstID_.insert(std::make_pair(hdr->sobTime, *hdr));
//			}

			if (hdr->eobTime != 0) {
				std::lock_guard<std::mutex> lock(mapMutex_);
				eobDataBySobTs_.insert(std::make_pair(hdr->sobTime, *hdr));
			}

			if (hdr->sobTime == 0) {
				LOG_ERROR("Handled a 0 SOB");
				return;
			} else {
				std::lock_guard<std::mutex> lock(mapMutex_);
				sobDataBySobTs_.insert(std::make_pair(hdr->sobTime, *hdr));
			}

			lastSobTs_ = hdr->sobTime;


			//Cleaning the map
			{
				std::lock_guard<std::mutex> lock(mapMutex_);
				if (sobDataBySobTs_.size() > 5) {
					for(auto & bi : sobDataBySobTs_) {
						if (lastSobTs_ - bi.first > 60*60*5) {//Five hours
							//LOG_INFO("Index " <<   bi.first << " deleted from the Burst info map");
							sobDataBySobTs_.erase(bi.first);
						} else {
							break;
						}
					}
				}
			}

			//Cleaning the map
			{
				std::lock_guard<std::mutex> lock(mapMutex_);
				if (eobDataBySobTs_.size() > 5) {
					for(auto & bi : eobDataBySobTs_) {
						if (lastSobTs_ - bi.first > 60*60*5) {
							//LOG_INFO("Index " <<   bi.first << " deleted from the Burst info map");
							eobDataBySobTs_.erase(bi.first);
						} else {
							break;
						}
					}
				}
			}
		}
	}

	BurstTimeInfo getInfoSOB(u_int32_t sobTs) {
		std::lock_guard<std::mutex> lock(mapMutex_);
		return sobDataBySobTs_.at(sobTs);
	}

	BurstTimeInfo getInfoEOB(u_int32_t sobTs) {
		std::lock_guard<std::mutex> lock(mapMutex_);
		return eobDataBySobTs_.at(sobTs);
	}
	std::map<u_int32_t, BurstTimeInfo> getAllInfoEOB() {
			std::lock_guard<std::mutex> lock(mapMutex_);
			return eobDataBySobTs_;
	}

	BurstTimeInfo getInfoByBurst(uint32_t burstID) {
		std::lock_guard<std::mutex> lock(mapMutex_);
		for (auto bi = sobDataBySobTs_.rbegin(); bi != sobDataBySobTs_.rend(); ++bi) {
			LOG_INFO(bi->first);
			if (bi->second.burstID == burstID) {
				return bi->second;
			}
		}
		//throw; //Need to be catched
	}


//	//for (auto pair : eventsBySobTsByID) {
//		for (auto event = eventsBySobTsByID.rbegin(); event!=eventsBySobTsByID.rend(); ++event ) {
//
//			//eobCollector_.getBurstInfoSOB(pair.first)
//		    uint32_t burstID = reinterpret_cast<EVENT_HDR*> ( ((zmq::message_t*) event->second.begin()->second)->data())->burstID;
//
//			stream << burstID << ":"<< event->second.size() << ";";
//			//stream << event << ":" << pair.second.size() << ";";
//		}
//		return stream.str();
//



private:
	std::map<u_int32_t, BurstTimeInfo> sobDataBySobTs_;
	std::map<u_int32_t, BurstTimeInfo> eobDataBySobTs_;
	std::mutex mapMutex_;
	std::atomic<int32_t> lastSobTs_;
};

}
}



#endif /* DIM_BURSTINFO_H_ */
