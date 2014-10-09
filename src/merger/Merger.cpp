/*
 * Merger.cpp
 *
 *  Created on: Jul 6, 2012
 *      Author: Jonas Kunze <kunze.jonas@gmail.com>
 */

#include <fstream>
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <structs/Event.h>

#include "../options/Options.h"

#include "Merger.hpp"

namespace na62 {
namespace merger {

Merger::Merger() :
		currentRunNumber_(Options::RUN_NUMBER), nextBurstSOBtimestamp_(0) {

	eobCollector_.run();
}

void Merger::addPacket(EVENT_HDR* event) {
	uint32_t burstID = event->burstID;
	std::lock_guard<std::mutex> lock(eventMutex);
	if (eventsByBurstByID[burstID].size() == 0) {
		if (nextBurstSOBtimestamp_ == 0) {
			/*
			 * Give it another try (at startup it can take a while until we get the message from dim...
			 */
			usleep(1000);
			if (nextBurstSOBtimestamp_ == 0) {
				std::cerr << "Received event even though the SOB is not defined yet. Dropping data!" << std::endl;
				delete[] event;
				return;
			}
		}
		dim::EobCollector::setCurrentBurstID(burstID);
		/*
		 * Only one thread will start the EOB checking thread
		 */
		if (newBurstMutex.try_lock()) {
			handle_newBurst(burstID);
			newBurstMutex.unlock();
		}

		SOBtimestampByBurst[burstID] = nextBurstSOBtimestamp_;
	}
	event->SOBtimestamp = SOBtimestampByBurst[burstID];
	eventsByBurstByID[burstID][event->eventNum] = event;

}

void Merger::handle_newBurst(uint32_t newBurstID) {
	std::cout << "New burst: " << newBurstID << std::endl;
	boost::thread(boost::bind(&Merger::startBurstControlThread, this, newBurstID));
	runNumberByBurst[newBurstID] = currentRunNumber_;
}

void Merger::startBurstControlThread(uint32_t& burstID) {
	size_t lastEventNum = -1;
	do {
		lastEventNum = eventsByBurstByID[burstID].size();
		sleep(Options::TIMEOUT);
	} while (eventsByBurstByID[burstID].size() > lastEventNum);
	std::cout << "Finishing burst " << burstID << " : " << eventsByBurstByID[burstID].size() << " because of normal timeout." << std::endl;
	handle_burstFinished(burstID);
}

void Merger::handle_burstFinished(uint32_t finishedBurstID) {
	std::lock_guard<std::mutex> lock(newBurstMutex);

	std::map<uint32_t, EVENT_HDR*> &burst = eventsByBurstByID[finishedBurstID];

	EVENT_HDR* oldEobEvent = (--burst.end())->second; // Take the last event as EOB event
	EVENT_HDR* eobEvent = eobCollector_.addEobDataToEvent(oldEobEvent);

	eventsByBurstByID[finishedBurstID][eobEvent->eventNum] = eobEvent;

	saveBurst(burst, finishedBurstID);
	eventsByBurstByID.erase(finishedBurstID);
}

void Merger::saveBurst(std::map<uint32_t, EVENT_HDR*>& eventByID, uint32_t& burstID) {
	uint32_t runNumber = runNumberByBurst[burstID];
	runNumberByBurst.erase(burstID);

	std::string fileName = generateFileName(runNumber, burstID, 0);
	std::string tmpName;
	std::string filePath = Options::STORAGE_DIR + "/" + fileName;
	std::cout << "Writing file " << filePath << std::endl;

	int numberOfEvents = eventByID.size();
	if (numberOfEvents == 0) {
		std::cerr << "No event received for burst " << burstID << std::endl;
		return;
	}

	if (boost::filesystem::exists(filePath)) {
		std::cerr << "File already exists: " << filePath << std::endl;
		int counter = 2;
		tmpName = generateFileName(runNumber, burstID, counter);
		while (boost::filesystem::exists(Options::STORAGE_DIR + "/" + tmpName)) {
			std::cerr << "File already exists: " << tmpName << std::endl;
			tmpName = generateFileName(runNumber, burstID, ++counter);
		}

		std::cerr << "Instead writing file: " << tmpName << std::endl;
		fileName = tmpName;
		filePath = Options::STORAGE_DIR + "/" + fileName;
	}

	std::ofstream myfile;
	myfile.open(filePath.data(), std::ios::out | std::ios::trunc | std::ios::binary);

	if (!myfile.good()) {
		std::cerr << "Unable to write to file " << filePath << std::endl;
		// carry on to free the memory. myfile.write will not throw!
	}

	size_t bytes = 0;
	for (auto pair : eventByID) {
		myfile.write((char*) pair.second, pair.second->length * 4);
		bytes += pair.second->length * 4;

		delete[] pair.second;
	}
	myfile.close();
	system(std::string("chown na62cdr:vl " + filePath).data());

	writeBKMFile(filePath, fileName, bytes);

	std::cout << "Wrote burst " << burstID << " with " << numberOfEvents << " events and " << bytes << " bytes" << std::endl;
}

void Merger::writeBKMFile(std::string dataFilePath, std::string fileName, size_t fileLength) {
	time_t rawtime;
	struct tm * timeinfo;
	char timeString[24];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(timeString, 64, "%d-%m-%y_%H:%M:%S", timeinfo);

	std::string BKMFilePath = Options::BKM_DIR + "/" + fileName;

	std::ofstream BKMFile;
	BKMFile.open(BKMFilePath.data(), std::ios::out | std::ios::trunc);

	if (!BKMFile.good()) {
		std::cerr << "Unable to write to file " << BKMFilePath << std::endl;
		return;
	}

	BKMFile.write(dataFilePath.data(), dataFilePath.length());
	BKMFile.write("\n", 1);

	std::string sizeLine = "size: " + boost::lexical_cast<std::string>(fileLength);
	BKMFile.write(sizeLine.data(), sizeLine.length());
	BKMFile.write("\n", 1);

	std::string dateLine = "datetime: " + std::string(timeString);
	BKMFile.write(dateLine.data(), dateLine.length());
	BKMFile.write("\n", 1);

	BKMFile.close();

	system(std::string("chown na62cdr:vl " + BKMFilePath).data());

	std::cout << "Wrote BKM file " << BKMFilePath << std::endl;
}

std::string Merger::generateFileName(uint32_t runNumber, uint32_t burstID, uint32_t duplicate) {
	char buffer[64];

	if (duplicate == 0) {
		sprintf(buffer, "cdr%02u%06u-%04u.dat", Options::MERGER_ID, runNumber, burstID);
	} else {
		sprintf(buffer, "cdr%02u%06u-%04u_%u.dat", Options::MERGER_ID, runNumber, burstID, duplicate);
	}
	return std::string(buffer);
}

} /* namespace merger */
} /* namespace na62 */
