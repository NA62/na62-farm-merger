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
#include <zmq.hpp>
#include <structs/Event.h>

#include "../options/Options.h"
#include "../utils/Utils.h"

namespace na62 {
namespace dim {

uint EobCollector::currentBurstID_ = 0;

EobCollector::EobCollector() {

}

EobCollector::~EobCollector() {
// TODO Auto-generated destructor stub
}

EobDataHdr* EobCollector::getData(DimInfo* dimInfo) {
	char* data = (char*) dimInfo->getData();
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

		/*
		 * Start a thread that will sleep a while and read the EOB services afterwards
		 */
		boost::thread([this, eob]() {
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

					std::lock_guard<std::mutex> lock(eobCallbackMutex_);

					// Delete old data
			eobDataByBurstIDBySourceID.erase(currentBurstID_);

			/*
			 * Store data of all services
			 */
			for(auto serviceNameAndService: eobInfoByName_) {
				EobDataHdr* hdr = getData(serviceNameAndService.second);
				if (hdr != nullptr && hdr->eobTimestamp==eob) {
					LOG(INFO) << "Storing data of service " << serviceNameAndService.second->getName() << " with " << hdr->length << " words and detector ID " << (int) hdr->detectorID << " for burst " << currentBurstID_;
					eobDataByBurstIDBySourceID[currentBurstID_][hdr->detectorID].push_back(hdr);
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

zmq::message_t* EobCollector::addEobDataToEvent(zmq::message_t* eventMessage) {
	EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

	if (eobDataByBurstIDBySourceID.count(event->burstID) == 0) {
		LOG(ERROR)<<"Trying to write EOB data of burst " << event->burstID << " even though it does not exist";
		return eventMessage;
	}

	/*
	 * The new event will have the following number of Bytes if all sources are active and therefore all dim data will be added to the event
	 */
	uint maxEventBufferSize = event->length * 4;

	// The actual new size will be stored here
	uint newEventSize = maxEventBufferSize;

	auto eobDataMap = eobDataByBurstIDBySourceID[event->burstID];

	if (eobDataMap.size() == 0) {
		return eventMessage;
	}

	EVENT_DATA_PTR* oldEventPointerTable = event->getDataPointer();

	/*
	 * Calculate the new event length by adding the lengths of all EOB-dim data
	 */
	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {
		EVENT_DATA_PTR sourceIdAndOffset = oldEventPointerTable[sourceNum];

		/*
		 * Sum all EOB-Dim data and add it to the eventBufferSize
		 */
		uint additionalWordsForThisSource = 0;
		for (EobDataHdr* hdr : eobDataMap[sourceIdAndOffset.sourceID]) {
			additionalWordsForThisSource += hdr->length;
		}
		maxEventBufferSize += additionalWordsForThisSource * 4;
	}

	/*
	 * Check if any dim-EOB data was found
	 */
	if (maxEventBufferSize == event->length * 4) {
		return eventMessage;
	}

	zmq::message_t* newEventMessage = new zmq::message_t(maxEventBufferSize);
	char* eventBuffer = (char*) newEventMessage->data();
	struct EVENT_HDR* header = (struct EVENT_HDR*) eventBuffer;

	EVENT_DATA_PTR* newEventPointerTable = header->getDataPointer();

	uint oldEventPtr = 0;
	uint newEventPtr = 0;

	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {
		EVENT_DATA_PTR sourceIdAndOffset = oldEventPointerTable[sourceNum];

		/*
		 * Search for a sourceID of which we have dim-eob data
		 */
		if (eobDataMap.count(sourceIdAndOffset.sourceID) == 0) {
			continue;
		}

		uint bytesToCopyFromOldEvent = 0;
		if (sourceNum != event->numberOfDetectors - 1) {
			EVENT_DATA_PTR nextSourceIdAndOffset = oldEventPointerTable[sourceNum + 1];

			/*
			 * Copy original event
			 */
			bytesToCopyFromOldEvent = nextSourceIdAndOffset.offset * 4 - oldEventPtr;
		} else {
			/*
			 * The last fragment goes from oldEventPtr to the trailer
			 */
			bytesToCopyFromOldEvent = (event->length * 4 - sizeof(EVENT_TRAILER)) - oldEventPtr;
		}

		memcpy(eventBuffer + newEventPtr, ((char*) event) + oldEventPtr, bytesToCopyFromOldEvent);
		oldEventPtr += bytesToCopyFromOldEvent;
		newEventPtr += bytesToCopyFromOldEvent;

		/*
		 * Copy all EOB data of this sourceID and delete the EOB data afterwards
		 */
		uint numberOfBytesAdded = 0;
		for (EobDataHdr* data : eobDataMap[sourceIdAndOffset.sourceID]) {
			std::cout << "Writing EOB-Data from dim for sourceID " << (int) sourceIdAndOffset.sourceID << std::endl;

			newEventSize += data->length * 4;

			memcpy(eventBuffer + newEventPtr, data, data->length * 4);
			numberOfBytesAdded += data->length * 4;
			delete data;
		}
		eobDataMap.erase(sourceIdAndOffset.sourceID);

		newEventPtr += numberOfBytesAdded;

		/*
		 * The data of following sourceIDs will be shifted -> increase their pointer offsets
		 */
		for (int followingSource = sourceNum + 1; followingSource != event->numberOfDetectors; followingSource++) {
			newEventPointerTable[sourceNum].offset += numberOfBytesAdded;
		}
	}

	/*
	 * Copy the rest including the trailer (or only the trailer if the last fragment has EOB data
	 */
	memcpy(eventBuffer + newEventPtr, ((char*) event) + oldEventPtr, event->length * 4 - oldEventPtr);

	/*
	 * Overwrite the new length. It's already aligned to 32 bits as the EOB only stores 4-byte length words
	 */
	header->length = newEventSize / 4;

	delete eventMessage;
	return newEventMessage;
}
}
}
