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

void Merger::addPacket(EVENT& event) {
//	boost::lock_guard<boost::mutex> lock(eventMutex);
	eventsByIDByBurst[event.hdr->burstID][event.hdr->eventNum] = event;
//	delete[] event.data;
}

void Merger::handle_newBurst(std::string address, uint32_t newBurstID) {
		burstIDsByConnection_[address] = newBurstID;
		std::cerr << "New burst: " << newBurstID << std::endl;
	}

void Merger::handle_burstFinished(std::string address, uint32_t finishedBurstID) {
	boost::lock_guard<boost::mutex> lock(newBurstMutex);

	uint32_t lastBurstID = burstIDsByConnection_[address];
	burstIDsByConnection_[address] = finishedBurstID;

	std::map<std::string, uint32_t>::const_iterator itr;

	bool allConnectionsDone = true;
	for (itr = burstIDsByConnection_.begin(); itr != burstIDsByConnection_.end(); ++itr) {
		if ((*itr).second != finishedBurstID) {
			allConnectionsDone = false;
			break;
		}
	}

	std::cerr << "Finished burst: " << lastBurstID << std::endl;

	if (allConnectionsDone) {
		std::map<uint32_t, EVENT> lastBurst = eventsByIDByBurst[lastBurstID];
		saveBurst(lastBurst, lastBurstID);
		eventsByIDByBurst.erase(lastBurstID);
	}
}

void Merger::saveBurst(std::map<uint32_t, EVENT>& eventByID, uint32_t& burstID) {
	if(Options::VERBOSE){
		print();
	}
	std::string fileName = generateFileName(burstID);
	std::cerr << "Writing file " << fileName << std::endl;

	int numberOfEvents = eventByID.size();
	if(numberOfEvents==0){
		std::cerr << "No event received for burst " << burstID << std::endl;
		return;
	}
	ofstream myfile;
	myfile.open(fileName.data(), ios::out | ios::trunc | ios::binary);

	std::map<uint32_t, EVENT>::const_iterator itr;

	size_t bytes = 0;
	for (itr = eventByID.begin(); itr != eventByID.end(); ++itr) {
		myfile.write((char*) (*itr).second.hdr, sizeof(struct EVENT_HDR));
		myfile.write((char*) (*itr).second.data, (*itr).second.hdr->length * 4-sizeof(struct EVENT_HDR)); // write
		bytes += (*itr).second.hdr->length * 4;

		delete[] (*itr).second.hdr;
		delete[] (*itr).second.data;

	}
	myfile.close();

	std::cerr << "Wrote burst " << burstID << " with " << numberOfEvents << " events and " << bytes << " bytes" << std::endl;
}

std::string Merger::generateFileName(uint32_t& burstID) {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[64];
	char timeString[24];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(timeString, 64, "%d-%m-%y_%H:%M:%S", timeinfo);
	sprintf(buffer, "%i-%s", burstID, timeString);
	return Options::STORAGE_DIR+"/"+std::string(buffer);
}

} /* namespace merger */
} /* namespace na62 */
