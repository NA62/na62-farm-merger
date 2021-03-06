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
namespace merger {

//uint EobCollector::currentBurstID_ = 0;

EobCollector::EobCollector() {

}

EobCollector::~EobCollector() {
	// TODO Auto-generated destructor stub
}

EobDataHdr* EobCollector::getData(DimInfo* dimInfo) {
	char* data = (char*) dimInfo->getData();
	uint dataLength = dimInfo->getSize();

	if (dataLength < sizeof(EobDataHdr)) {
		LOG_ERROR("EOB Service " << dimInfo->getName() << " does not contain enough data: Found only "
				<< dataLength << " B but at least " << sizeof(EobDataHdr) << " B are needed to store the header ");
	} else {
		EobDataHdr* hdr = (EobDataHdr*) data;
		if (hdr->length * 4 != dataLength) {
			LOG_ERROR("EOB Service " << dimInfo->getName() <<" does not contain the right number of bytes: The header says "
					<< hdr->length*4 << " but DIM stores " << dataLength);
		} else {
			char* buff = new char[dataLength];
			memcpy(buff, data, dataLength);
			return (EobDataHdr*) buff;
		}
	}
	return nullptr;
}

void EobCollector::run() {
	LOG_INFO("Starting EOBCollector Thread");
	is_running_ = true;
	while(is_running_) {
		{
			std::lock_guard<std::mutex> lock(eobCallbackMutex_);

			DimBrowser dimBrowser;
			char *service, *format;
			dimBrowser.getServices("NA62/EOB/*");
			while (dimBrowser.getNextService(service, format)) {
				std::string serviceName(service);
				if (serviceName.find("NA62/EOB/") != std::string::npos) {
					if(eobInfoByName_.find(serviceName) == eobInfoByName_.end()) {
						LOG_INFO("New service found: " << serviceName );
						eobInfoByName_[serviceName] = new EobDimInfo(serviceName);
					}
				} else {
					LOG_ERROR("Dim browser match a wrong service: " << serviceName );
				}
			}
		}
		for (uint cycle = 0; cycle < 10; cycle++) {
			if (is_running_) {
				sleep(1);
			}
		}
	}
	LOG_INFO("Exiting from EOBCollector");
}
void EobCollector::stop() {
	LOG_INFO("Stopping EOBCollector");
	is_running_ = false;
}

//zmq::message_t* EobCollector::addEobDataToEvent(zmq::message_t* eventMessage) {
char* EobCollector::addEobDataToEvent(zmq::message_t* eventMessage) {
	std::lock_guard<std::mutex> lock(eobCallbackMutex_);
	EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());
	u_int32_t ts =  0;

	try {
		BurstTimeInfo bi = burstInfo_.getInfoEOB(event->burstID);
		ts = bi.eobTime;
	} catch(std::exception&e) {
		LOG_ERROR("DIM burst time data for Burst " << (int) event->burstID << " not found.");
		return  (char*) eventMessage->data();
	}

	std::map<uint8_t, std::vector<EobDataHdr*>> eobDataBySourceID;

	// Fill all DIM information by detector
	for (auto eobService: eobInfoByName_) {
		auto eobInfo = eobService.second->getInfo(ts);
		if (eobInfo == NULL) {
			LOG_WARNING("No information found with ts " << (int) ts << " for service " << eobService.first);
			continue;
		}
		eobDataBySourceID[eobInfo->detectorID].push_back(eobInfo);
		eobService.second->clearInfo(ts);
	}

	if (eobDataBySourceID.size() == 0) {
		LOG_ERROR("Trying to write dim-EOB data of burst " << event->burstID << " even though none has been received");
		return (char*) eventMessage->data();
	}

	/*
	 * The new event will have the following number of Bytes if all sources are active and therefore all dim data will be added to the event
	 */
	uint newEventBufferSize = event->length * 4;

	// The actual new size will be stored here
	uint newEventSize = newEventBufferSize;

	if (event->getL0TriggerTypeWord() != TRIGGER_L0_EOB) {
		for (auto sourceIDAndDataVector : eobDataBySourceID) {
			for (auto data : sourceIDAndDataVector.second) {
				delete[] data;
			}
			eobDataBySourceID.erase(sourceIDAndDataVector.first);
		}
		LOG_ERROR("The event to which we should add EOB data is not a EOB event");
		return (char*) eventMessage->data();
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
				sourceIdAndOffset.offset += totalEOBlength / 4;
			}
		}
	}

	/*
	 * Check if any dim-EOB data was found
	 */
	if (newEventBufferSize == event->length * 4) {
		return (char*) eventMessage->data();
	}

	char * allEOBpacket = new char[totalEOBlength];
	char *EOBpnt = allEOBpacket;
	for (int sourceNum = 0; sourceNum != event->numberOfDetectors; sourceNum++) {

		EVENT_DATA_PTR sourceIdAndOffset = sourceIdAndOffsets[sourceNum];

		if (eobDataBySourceID.count(sourceIdAndOffset.sourceID) == 0) {
			LOG_INFO("No extra EOB data for detector 0x" << std::hex << (int) sourceIdAndOffset.sourceID << std::dec);
			continue;
		}

		/*
		 * Copy all EOB data of this sourceID and delete the EOB data afterwards
		 */
		for (EobDataHdr* data : eobDataBySourceID[sourceIdAndOffset.sourceID]) {
			LOG_INFO("Copying EOB-Data from dim for sourceID 0x"<< std::hex << (int) sourceIdAndOffset.sourceID << std::dec);
			memcpy(EOBpnt, data, data->length * 4);

			EOBpnt += data->length * 4;

			delete[] data;
		}
		eobDataBySourceID.erase(sourceIdAndOffset.sourceID);
	}

//	zmq::message_t* newEventMessage = new zmq::message_t(newEventBufferSize);
//	struct EVENT_HDR* header = (struct EVENT_HDR*) newEventMessage->data();
//	char* eventBuffer = reinterpret_cast<char*>(newEventMessage->data());

	char* newEventMessage = new char[newEventBufferSize];
	struct EVENT_HDR* header = (struct EVENT_HDR*) newEventMessage;
	char* eventBuffer = reinterpret_cast<char*>(newEventMessage);




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
			LOG_INFO("Writing EOB-Data to NSTD block");
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
