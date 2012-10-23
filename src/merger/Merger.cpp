/*
 * Merger.cpp
 *
 *  Created on: Jul 6, 2012
 *      Author: root
 */

#include <fstream>
#include <iostream>

#include "../options/Options.h"
#include "Merger.hpp"

namespace na62 {
namespace merger {

using namespace std;
Merger::Merger() :
		currentRunNumber_(Options::RUN_NUMBER) {
}

void Merger::addPacket(EVENT& event) {
	boost::lock_guard<boost::mutex> lock(eventMutex);
	if (eventsByIDByBurst[event.hdr->burstID].size() == 0) {
		if (newBurstMutex.try_lock()) {
			handle_newBurst(event.hdr->burstID);
			newBurstMutex.unlock();
		}
	}
	eventsByIDByBurst[event.hdr->burstID][event.hdr->eventNum] = event;

}

void Merger::handle_newBurst(uint32_t newBurstID) {
	std::cout << "New burst: " << newBurstID << std::endl;
	boost::thread(boost::bind(&Merger::startBurstControlThread, this, newBurstID));
	runNumberByBurst[newBurstID] = currentRunNumber_;
}

void Merger::startBurstControlThread(uint32_t& burstID) {
	size_t lastEventNum = -1;
	do {
		lastEventNum = eventsByIDByBurst[burstID].size();
		sleep(8);
	} while (eventsByIDByBurst[burstID].size() > lastEventNum);
	std::cout << "Finishing burst " << burstID << " : " << eventsByIDByBurst[burstID].size() << " because of normal timeout." << std::endl;
	handle_burstFinished(burstID);
}

void Merger::handle_burstFinished(uint32_t finishedBurstID) {
	boost::lock_guard<boost::mutex> lock(newBurstMutex);

	std::map<uint32_t, EVENT> burst = eventsByIDByBurst[finishedBurstID];
	saveBurst(burst, finishedBurstID);
	eventsByIDByBurst.erase(finishedBurstID);
}

void Merger::saveBurst(std::map<uint32_t, EVENT>& eventByID, uint32_t& burstID) {
	if (Options::VERBOSE) {
		print();
	}
	uint32_t runNumber = runNumberByBurst[burstID];
	runNumberByBurst.erase(burstID);

	std::string fileName = generateFileName(runNumber, burstID);
	std::cout << "Writing file " << fileName << std::endl;

	int numberOfEvents = eventByID.size();
	if (numberOfEvents == 0) {
		std::cerr << "No event received for burst " << burstID << std::endl;
		return;
	}
	ofstream myfile;
	myfile.open(fileName.data(), ios::out | ios::trunc | ios::binary);

	if (!myfile.good()) {
		std::cerr << "Unable to write to file " << fileName << std::endl;
		// all carry on to free the memory. myfile.write will not throw!
	}

	std::map<uint32_t, EVENT>::const_iterator itr;

	size_t bytes = 0;
	for (itr = eventByID.begin(); itr != eventByID.end(); ++itr) {
		myfile.write((char*) (*itr).second.hdr, sizeof(struct EVENT_HDR));
		myfile.write((char*) (*itr).second.data, (*itr).second.hdr->length * 4 - sizeof(struct EVENT_HDR)); // write
		bytes += (*itr).second.hdr->length * 4;

		delete[] (*itr).second.hdr;
		delete[] (*itr).second.data;

	}
	myfile.close();

	std::cout << "Wrote burst " << burstID << " with " << numberOfEvents << " events and " << bytes << " bytes" << std::endl;
}

std::string Merger::generateFileName(uint32_t runNumber, uint32_t burstID) {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[64];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	sprintf(buffer, "cdr(%2d)(%6d)-(%4d).dat", Options::MERGER_ID, runNumber, burstID);
	return Options::STORAGE_DIR + "/" + std::string(buffer);
}

} /* namespace merger */
} /* namespace na62 */
