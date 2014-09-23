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
}
}
