/*
 * EobCollector.cpp
 *
 *  Created on: Sep 23, 2014
 *      Author: Jonas Kunze <kunze.jonas@gmail.com>
 */

#include "EobCollector.h"

#include <iostream>
#include <glog/logging.h>
#include <boost/thread.hpp>
#include <thread>
#include <mutex>

#include <structs/Event.h>

#include "../options/Options.h"
#include "../utils/Utils.h"

namespace na62 {
namespace dim {
EobCollector::EobCollector() {

}

EobCollector::~EobCollector() {
// TODO Auto-generated destructor stub
}

EobDataHdr* EobCollector::getData(DimInfo* dimInfo) {
	char* data = dimInfo->getString();
	uint dataLength = dimInfo->getSize();

	if (dataLength < sizeof(EobDataHdr)) {
		LOG(ERROR)<<"EOB Service "<<dimInfo->getName()<<" does not contain enough data: Found only " <<
		dataLength << " B but at least " << sizeof(EobDataHdr) <<
		" B are needed to store the header ";
	} else {
		EobDataHdr* hdr = (EobDataHdr*) data;
		if (hdr->length*4 != dataLength) {
			LOG(ERROR)<< "EOB Service "<<dimInfo->getName()<<" does not contain the right number of bytes: The header says " <<
			hdr->length*4 << " but DIM stores " << dataLength;
		} else {
			return (EobDataHdr*) data;
		}
	}
	return nullptr;
}

void EobCollector::run() {
	dimListener_.registerEobListener([this](uint eob) {
		/*
		 * This is executed after every eob update
		 */
		if(eob==0) {
			return;
		}

		uint burstID = dimListener_.getBurstNumber();

		/*
		 * Start a thread that will sleep a while and read the EOB services afterwards
		 */
		boost::thread([this, eob, burstID]() {
					/*
					 * Update list of DimInfos within one mutex protected scope
					 */
					{	std::lock_guard<std::mutex> lock(eobCallbackMutex_);

						DimBrowser dimBrowser;
						char *service, *format;
						dimBrowser.getServices("NA62/EOB/*");
						while (dimBrowser.getNextService(service, format)) {
							std::string serviceName(service);

							if(eobInfoByName_.count(serviceName) == 0) {
								LOG(INFO) << "New service found: " << serviceName << "\t" << format;
								eobInfoByName_[std::string(serviceName)] = new DimInfo(service, -1);
							}
						}
					}

					/*
					 * Wait for the services to update the data
					 */
					usleep(na62::merger::Options::EOB_COLLECTION_TIMEOUT*1000);

					for(auto serviceNameAndService: eobInfoByName_) {
						EobDataHdr* hdr = getData(serviceNameAndService.second);
						if (hdr != nullptr && hdr->eobTimestamp==eob) {
							eobDataBySourceIDByBurstID[burstID][hdr->detectorID].push_back(hdr);
						} else {
							if( hdr != nullptr) {
								LOG(ERROR) << "Found EOB with TS " << hdr->eobTimestamp << " after receiving EOB TS " << eob;
							}
							delete hdr;
						}
					}
				});

	});
}

EVENT_HDR* EobCollector::addEobDataToEvent(EVENT_HDR* event) {
	if (eobDataBySourceIDByBurstID.count(event->burstID) == 0) {
		LOG(ERROR)<<"Trying to write EOB data of burst " << event->burstID << " even though it does not exist";
		return nullptr;
	}

	uint eventBufferSize = event->length*4;

	auto eobDataMap = eobDataBySourceIDByBurstID[event->burstID];

	if(eobDataMap.size()==0) {
		return nullptr;
	}

	/*
	 * Calculate the new event length
	 */
	for(auto& sourceIDAndhdrs: eobDataMap) {
		for(auto hdr: sourceIDAndhdrs.second) {
			int subeventLength = hdr->length;
			/*
			 * 4 Byte alignment for every event
			 */
			if (subeventLength % 4 != 0) {
				subeventLength = subeventLength + (4-subeventLength % 4);
			}

			eventBufferSize += subeventLength;
		}
	}

	if(eventBufferSize == event->length*4) {
		return nullptr;
	}

	char* eventBuffer = new char[eventBufferSize];
	struct EVENT_HDR* header = (struct EVENT_HDR*) eventBuffer;

	uint oldEventPtr = 0;
	uint newEventPtr = 0;

	EVENT_DATA_PTR* sourceIdAndOffsets = event->getDataPointer();
	for(int sourceNum=0; sourceNum!=event->numberOfDetectors; sourceNum++) {
		EVENT_DATA_PTR sourceIdAndOffset = sourceIdAndOffsets[sourceNum];

		if(eobDataMap.count(sourceIdAndOffset.sourceID)==0) {
			continue;
		}

		uint bytesToCopyFromOldEvent = 0;
		if(sourceNum!=event->numberOfDetectors-1) {
			EVENT_DATA_PTR nextSourceIdAndOffset = sourceIdAndOffsets[sourceNum+1];

			/*
			 * Copy original event
			 */
			bytesToCopyFromOldEvent = nextSourceIdAndOffset.offset*4-oldEventPtr;
		} else {
			/*
			 * The last fragment goes from oldEventPtr to the trailer
			 */
			bytesToCopyFromOldEvent = (event->length*4-sizeof(EVENT_TRAILER))-oldEventPtr;
		}

		memcpy(eventBuffer+newEventPtr, event+oldEventPtr, bytesToCopyFromOldEvent);
		oldEventPtr += bytesToCopyFromOldEvent;
		newEventPtr += bytesToCopyFromOldEvent;

		/*
		 * Copy all EOB data of this sourceID and delete the EOB data afterwards
		 */
		for(EobDataHdr* data: eobDataMap[sourceIdAndOffset.sourceID]) {
			memcpy(eventBuffer+newEventPtr, data, data->length);
			newEventPtr+=data->length;
			delete data;
		}
		eobDataMap.erase(sourceIdAndOffset.sourceID);
	}

	/*
	 * Copy the rest including the trailer (or only the trailer if the last fragment has EOB data
	 */
	memcpy(eventBuffer+newEventPtr, event+oldEventPtr, event->length*4-oldEventPtr);

	delete[] event;

	return header;
}

//EVENT_HDR* EobCollector::getEobEvent(uint burstID) {
//	if (eobDataBySourceIDByBurstID.count(burstID) == 0) {
//		LOG(ERROR)<<"Trying to write EOB data of burst " << burstID << " even though it does not exist";
//		return nullptr;
//	}
//
//	uint eventBufferSize = sizeof(EVENT_HDR);
//
//	auto eobDataVector = eobDataBySourceIDByBurstID[burstID];
//
//	if(eobDataVector.size()==0) {
//		return nullptr;
//	}
//
//	for(EobDataHdr* hdr: eobDataVector) {
//		int subeventLength = hdr->length;
//		/*
//		 * 4 Byte alignment for every event
//		 */
//		if (subeventLength % 4 != 0) {
//			subeventLength = subeventLength + (4-subeventLength % 4);
//		}
//
//		eventBufferSize+=4/*pointer table*/+ subeventLength;
//	}
//
//	eventBufferSize += sizeof(EVENT_TRAILER);
//
//	char* eventBuffer = new char[eventBufferSize];
//
//	struct EVENT_HDR* header = (struct EVENT_HDR*) eventBuffer;
//
//	header->eventNum = 0xFFFFFF;
//	header->format = 0xFF;
//	// header->length will be written later on
//	header->burstID = burstID;
//	header->timestamp = eobDataVector[0]->eobTimestamp;
//	header->triggerWord = 0;
//	header->reserved1 = 0;
//	header->fineTime = 0;
//	header->numberOfDetectors = eobDataVector.size();
//	header->reserved2 = 0;
//	header->processingID = 0;
//	header->SOBtimestamp = 0;// Will be set by the merger
//
//	uint32_t sizeOfPointerTable = 4 * header->numberOfDetectors;
//	uint32_t pointerTableOffset = sizeof(struct EVENT_HDR);
//	uint32_t eventOffset = sizeof(struct EVENT_HDR) + sizeOfPointerTable;
//
//	for(EobDataHdr* hdr: eobDataVector) {
//		uint32_t eventOffset32 = eventOffset / 4;
//
//		/*
//		 * Put the sub-detector into the pointer table
//		 */
//		std::memcpy(eventBuffer + pointerTableOffset, &eventOffset32, 3);
//		std::memset(eventBuffer + pointerTableOffset + 3, hdr->detectorID, 1);
//		pointerTableOffset += 4;
//
//		/*
//		 * Write the EOB data
//		 */
//		memcpy(eventBuffer + eventOffset ,
//				((char*)hdr)+sizeof(EobDataHdr), hdr->length );
//		eventOffset += hdr->length;
//
//		/*
//		 * 32-bit alignment
//		 */
//		if (eventOffset % 4 != 0) {
//			memset(eventBuffer + eventOffset, 0, eventOffset % 4);
//			eventOffset += eventOffset % 4;
//		}
//
//		delete[] hdr;
//	}
//
//	/*
//	 * Trailer
//	 */
//	EVENT_TRAILER* trailer = (EVENT_TRAILER*) (eventBuffer + eventOffset);
//	trailer->eventNum = header->eventNum;
//	trailer->reserved = 0;
//
//	const uint eventLength = eventOffset + 4/*trailer*/;
//	header->length = eventLength / 4;
//
//	assert(eventBufferSize == eventLength);
//
//	/*
//	 * Cleanup
//	 */
//	eobDataBySourceIDByBurstID.erase(burstID);
//
//	/*
//	 * Remove old EOB data not written by this merger
//	 */
//	for(auto pair: eobDataBySourceIDByBurstID) {
//		uint ts = pair.first;
//		if(ts < burstID) {
//			for(auto data: pair.second) {
//				delete[] data;
//			}
//			eobDataBySourceIDByBurstID.erase(ts);
//		}
//	}
//
//	return header;
//}
}
}
