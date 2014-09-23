/*
 * EobCollector.cpp
 *
 *  Created on: Sep 23, 2014
 *      Author: Jonas Kunze <kunze.jonas@gmail.com>
 */

#include "EobCollector.h"

#include <dim/dic.hxx>
#include <iostream>
#include <glog/logging.h>
#include <thread>

#include <structs/Event.h>

#include "../options/Options.h"

namespace na62 {
namespace dim {
EobCollector::EobCollector() {

}

EobCollector::~EobCollector() {
// TODO Auto-generated destructor stub
}

EobDataHdr* EobCollector::getDataHdr(char* serviceName) {
	DimUpdatedInfo dimInfo(serviceName, -1);
	void* data = dimInfo.getData();
	uint dataLength = dimInfo.getSize();
	if (dataLength < sizeof(EobDataHdr)) {
		LOG(ERROR)<<"EOB Service "<<serviceName<<" does not contain enough data";
	} else {
		EobDataHdr* hdr = (EobDataHdr*) data;
		if(hdr->length != dataLength) {
			LOG(ERROR)<< "EOB Service "<<serviceName<<" does not contain enough data";
		} else {
			return (EobDataHdr*) data;
		}
	}
	return nullptr;
}

void EobCollector::run() {
	std::thread* lastEobHandleThread;
	dimListener_.registerEobListener([this, &lastEobHandleThread/* call by reference! */](uint eob) {
		/*
		 * This is executed after every eob update
		 */
		if(eob==0) {
			return;
		}

		/*
		 * Start a thread that will sleep a while and read the EOB services afterwards
		 */
		std::thread* eobHandleThread = new std::thread([this, eob, lastEobHandleThread/* call by value! */]() {
					/*
					 * Wait until the last spawned thread has finished and delete it afterwards
					 */
					if(lastEobHandleThread!=nullptr) {
						lastEobHandleThread->join();
						delete lastEobHandleThread;
					}
					/*
					 * Wait for the services to update the data
					 */
					usleep(na62::merger::Options::EOB_COLLECTION_TIMEOUT*1000);

					DimBrowser dimBrowser;
					char *service, *format;
					dimBrowser.getServices("NA62/*/EOB");
					while (dimBrowser.getNextService(service, format)) {
						std::cout << "service found: " << service << "\t" << format << std::endl;

						EobDataHdr* hdr = getDataHdr(service);
						if (hdr != nullptr && hdr->eobTimestamp==eob) {
							eobDataByEobTimeStamp[eob].push_back(hdr);
						} else {
							delete hdr;
						}
					}
				});

		lastEobHandleThread = eobHandleThread;
	});
}

EVENT_HDR* EobCollector::getEobEvent(uint eobTimeStamp, uint burstID) {
	if (eobDataByEobTimeStamp.count(eobTimeStamp) == 0) {
		LOG(ERROR)<<"Trying to write EOB data with timestamp " << eobTimeStamp << " even though it does not exist";
	}

	uint eventBufferSize = sizeof(EVENT_HDR);

	auto eobDataVector = eobDataByEobTimeStamp[eobTimeStamp];
	for(EobDataHdr* hdr: eobDataVector) {
		int subeventLength = hdr->length;
		/*
		 * 4 Byte alignment for every event
		 */
		if (subeventLength % 4 != 0) {
			subeventLength = subeventLength + (4-subeventLength % 4);
		}

		eventBufferSize+=4/*pointer table*/+ subeventLength;
	}

	eventBufferSize += sizeof(EVENT_TRAILER);

	char* eventBuffer = new char[eventBufferSize];

	struct EVENT_HDR* header = (struct EVENT_HDR*) eventBuffer;

	header->eventNum = 0xFFFFFF;
	header->format = 0xFF;
	// header->length will be written later on
	header->burstID = burstID;
	header->timestamp = eobTimeStamp;
	header->triggerWord = 0;
	header->reserved1 = 0;
	header->fineTime = 0;
	header->numberOfDetectors = eobDataVector.size();
	header->reserved2 = 0;
	header->processingID = 0;
	header->SOBtimestamp = 0;// Will be set by the merger

	uint32_t sizeOfPointerTable = 4 * header->numberOfDetectors;
	uint32_t pointerTableOffset = sizeof(struct EVENT_HDR);
	uint32_t eventOffset = sizeof(struct EVENT_HDR) + sizeOfPointerTable;

	for(EobDataHdr* hdr: eobDataVector) {
		uint32_t eventOffset32 = eventOffset / 4;

		/*
		 * Put the sub-detector into the pointer table
		 */
		std::memcpy(eventBuffer + pointerTableOffset, &eventOffset32, 3);
		std::memset(eventBuffer + pointerTableOffset + 3, hdr->detectorID, 1);
		pointerTableOffset += 4;

		/*
		 * Write the EOB data
		 */
		memcpy(eventBuffer + eventOffset ,
				((char*)hdr)+sizeof(EobDataHdr), hdr->length );
		eventOffset += hdr->length;

		/*
		 * 32-bit alignment
		 */
		if (eventOffset % 4 != 0) {
			memset(eventBuffer + eventOffset, 0, eventOffset % 4);
			eventOffset += eventOffset % 4;
		}

		delete[] hdr;
	}

	/*
	 * Trailer
	 */
	EVENT_TRAILER* trailer = (EVENT_TRAILER*) (eventBuffer + eventOffset);
	trailer->eventNum = header->eventNum;
	trailer->reserved = 0;

	const uint eventLength = eventOffset + 4/*trailer*/;
	header->length = eventLength / 4;

	assert(eventBufferSize == eventLength);

	/*
	 * Cleanup
	 */
	eobDataByEobTimeStamp.erase(eobTimeStamp);

	/*
	 * Remove old EOB data not written by this merger
	 */
	for(auto pair: eobDataByEobTimeStamp) {
		uint ts = pair.first;
		if(ts < eobTimeStamp) {
			for(auto data: pair.second) {
				delete[] data;
			}
			eobDataByEobTimeStamp.erase(ts);
		}
	}

	return header;
}
}
}
