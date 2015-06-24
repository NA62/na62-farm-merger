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
#include <options/Logging.h>
#include <eventBuilding/SourceIDManager.h>

#include "../options/MyOptions.h"
#include <utils/Utils.h>

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
		LOG_ERROR<<"EOB Service "<<dimInfo->getName()<<" does not contain enough data: Found only " <<
		dataLength << " B but at least " << sizeof(EobDataHdr) <<
		" B are needed to store the header ";
	} else {
		EobDataHdr* hdr = (EobDataHdr*) data;
		if (hdr->length*4 != dataLength) {
			LOG_ERROR<< "EOB Service "<<dimInfo->getName()<<" does not contain the right number of bytes: The header says " <<
			hdr->length*4 << " but DIM stores " << dataLength;
		} else {
			char* buff = new char[dataLength];
			memcpy(buff, data, dataLength);
			return (EobDataHdr*) buff;
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
								LOG_INFO << "New service found: " << serviceName << "\t" << format;
								eobInfoByName_[std::string(serviceName)] = new DimInfo(service, -1);
							}
						}
					}

					/*
					 * Wait for the services to update the data
					 */
					usleep(MyOptions::GetInt(OPTION_EOB_COLLECTION_TIMEOUT)*1000);

					std::lock_guard<std::mutex> lock(eobCallbackMutex_);

					// Delete old data
			eobDataByBurstIDBySourceID.erase(currentBurstID_);

			/*
			 * Store data of all services
			 */
			for(auto serviceNameAndService: eobInfoByName_) {
				EobDataHdr* hdr = getData(serviceNameAndService.second);
				if (hdr != nullptr && hdr->eobTimestamp==eob) {
					LOG_INFO << "Storing data of service " << serviceNameAndService.second->getName() << " with " << hdr->length << " words and detector ID " << (int) hdr->detectorID << " for burst " << currentBurstID_;
					eobDataByBurstIDBySourceID[currentBurstID_][hdr->detectorID].push_back(hdr);
				} else {
					if( hdr != nullptr) {
						LOG_ERROR << "Found EOB with TS " << hdr->eobTimestamp << " after receiving EOB TS " << eob;
					}
					delete[] hdr;
				}
			}
		});

});
}

zmq::message_t* EobCollector::addEobDataToEvent(zmq::message_t* eventMessage) {
	EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

	if (eobDataByBurstIDBySourceID.count(event->burstID) == 0) {
		LOG_ERROR<<"Trying to write dim-EOB data of burst " << event->burstID << " even though none has been received";
		return eventMessage;
	}

	/*
	 * The new event will have the following number of Bytes if all sources are active and therefore all dim data will be added to the event
	 */
	uint newEventBufferSize = event->length * 4;

	// The actual new size will be stored here
	uint newEventSize = newEventBufferSize;

	auto eobDataBySourceID = eobDataByBurstIDBySourceID[event->burstID];

	if (eobDataBySourceID.size() == 0) {
		return eventMessage;
	}

	if (event->getL0TriggerTypeWord() != TRIGGER_L0_EOB) {
		for (auto sourceIDAndDataVector : eobDataBySourceID) {
			for (auto data : sourceIDAndDataVector.second) {
				delete[] data;
			}
			eobDataBySourceID.erase(sourceIDAndDataVector.first);
		}
		eobDataByBurstIDBySourceID.erase(event->burstID);
		return eventMessage;
	}

	EVENT_DATA_PTR* sourceIdAndOffsets = event->getDataPointer();

	EVENT_DATA_PTR* newEventPointerTable = new EVENT_DATA_PTR[event->numberOfDetectors];
	memcpy(newEventPointerTable, sourceIdAndOffsets, event->numberOfDetectors * sizeof(EVENT_DATA_PTR));

	/*
	 * Calculate the new event length by adding the lengths of all EOB-dim data
	 */
	uint totalEOBlength = 0;
	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {
		EVENT_DATA_PTR sourceIdAndOffset = sourceIdAndOffsets[sourceNum];

		if (eobDataBySourceID.count(sourceIdAndOffset.sourceID) == 0) {
			continue;
		}

		/*
		 * Sum all EOB-Dim data and add it to the eventBufferSize
		 */
		uint additionalWordsForThisSource = 0;
		for (auto hdr : eobDataBySourceID[sourceIdAndOffset.sourceID]) {
			additionalWordsForThisSource += hdr->length;
		}
		newEventBufferSize += additionalWordsForThisSource * 4;
		totalEOBlength += additionalWordsForThisSource * 4;
	}
	/*
	 * The data of following sourceIDs will be shifted -> increase their pointer offsets
	 */
	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {
		EVENT_DATA_PTR sourceIdAndOffset = sourceIdAndOffsets[sourceNum];

		if (sourceIdAndOffset.sourceID == SOURCE_ID_NSTD) {
			for (int followingSource = sourceNum + 1; followingSource != event->numberOfDetectors; followingSource++) {
				EVENT_DATA_PTR& sourceIdAndOffset = newEventPointerTable[followingSource];
				sourceIdAndOffset.offset += totalEOBlength;
			}
		}
	}

	/*
	 * Check if any dim-EOB data was found
	 */
	if (newEventBufferSize == event->length * 4) {
		return eventMessage;
	}

	char * allEOBpacket = new char[totalEOBlength];
	char *EOBpnt = allEOBpacket;
	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {

		EVENT_DATA_PTR sourceIdAndOffset = sourceIdAndOffsets[sourceNum];

		if (eobDataBySourceID.count(sourceIdAndOffset.sourceID) == 0) {
			continue;
		}

		/*
		 * Copy all EOB data of this sourceID and delete the EOB data afterwards
		 */
		for (EobDataHdr* data : eobDataBySourceID[sourceIdAndOffset.sourceID]) {
			LOG_INFO<< "Copying EOB-Data from dim for sourceID " << (int) sourceIdAndOffset.sourceID << ENDL;
			memcpy(EOBpnt, data, data->length * 4);

			EOBpnt += data->length * 4;

			delete[] data;
		}
		eobDataBySourceID.erase(sourceIdAndOffset.sourceID);
	}

	zmq::message_t* newEventMessage = new zmq::message_t(newEventBufferSize);
	struct EVENT_HDR* header = (struct EVENT_HDR*) newEventMessage->data();
	char* eventBuffer = reinterpret_cast<char*>(newEventMessage->data());

	uint oldEventPtr = 0;
	uint newEventPtr = 0;
	uint * saveEventPtr = 0;
	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {

		EVENT_DATA_PTR sourceIdAndOffset = sourceIdAndOffsets[sourceNum];

		uint bytesToCopyFromOldEvent = 0;
		if (sourceNum != event->numberOfDetectors - 1) {
			EVENT_DATA_PTR nextSourceIdAndOffset = sourceIdAndOffsets[sourceNum + 1];

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
		saveEventPtr = (uint *) (eventBuffer + newEventPtr);
		oldEventPtr += bytesToCopyFromOldEvent;
		newEventPtr += bytesToCopyFromOldEvent;

		/*
		 * Copy all EOB data to sourceID SOURCE_ID_NSTD EOB data
		 */

		if (sourceIdAndOffset.sourceID == SOURCE_ID_NSTD) {
			LOG_INFO<< "Writing EOB-Data to NSTD block" << ENDL;
			newEventSize += totalEOBlength;

			memcpy(eventBuffer + newEventPtr, allEOBpacket, totalEOBlength);
			newEventPtr += totalEOBlength;
			*saveEventPtr += totalEOBlength;
			delete[] allEOBpacket;
		}
	}
	/*
	 * Copy the rest including the trailer (or only the trailer if the last fragment has EOB data
	 */
	memcpy(eventBuffer + newEventPtr, ((char*) event) + oldEventPtr, event->length * 4 - oldEventPtr);

	/*
	 * Overwrite the pointer table with the corrected values
	 */
	memcpy(header->getDataPointer(), newEventPointerTable, header->numberOfDetectors * sizeof(EVENT_DATA_PTR));

	/*
	 * Overwrite the new length. It's already aligned to 32 bits as the EOB only stores 4-byte length words
	 */
	header->length = newEventSize / 4;

	delete eventMessage;

	return newEventMessage;

}
}
}
