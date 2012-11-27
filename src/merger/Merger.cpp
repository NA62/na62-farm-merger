/*
 * Merger.cpp
 *
 *  Created on: Jul 6, 2012
 *      Author: root
 */

#include <fstream>
#include <iostream>

#include "../options/Options.h"
#include<boost/filesystem.hpp>

#include "Merger.hpp"

namespace na62 {
namespace merger {

using namespace std;
Merger::Merger() :
		currentRunNumber_(Options::RUN_NUMBER), nextBurstSOBtimestamp_(0) {
}

void Merger::addPacket(EVENT& event) {
	uint32_t burstID = event.hdr->burstID;
	boost::lock_guard<boost::mutex> lock(eventMutex);
	if (eventsByIDByBurst[burstID].size() == 0) {
		/*
		 * Only one thread will start the EOB checking thread
		 */
		if (newBurstMutex.try_lock()) {
			handle_newBurst(burstID);
			newBurstMutex.unlock();
		}

		SOBtimestampByBurst[burstID] = nextBurstSOBtimestamp_;
	}
	event.hdr->SOBtimestamp = SOBtimestampByBurst[burstID];
	eventsByIDByBurst[burstID][event.hdr->eventNum] = event;

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
		sleep(Options::TIMEOUT);
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
	uint32_t runNumber = runNumberByBurst[burstID];
	runNumberByBurst.erase(burstID);

	std::string fileName = generateFileName(runNumber, burstID, 0);
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
		std::string tmpName;
		tmpName = generateFileName(runNumber, burstID, counter);
		while (boost::filesystem::exists(Options::STORAGE_DIR + "/" + tmpName)) {
			std::cerr << "File already exists: " << tmpName << std::endl;
			tmpName = fileName + "." + boost::lexical_cast<std::string>(++counter);
		}

		std::cerr << "Instead writing file: " << tmpName << std::endl;
		fileName = tmpName;
		filePath = Options::STORAGE_DIR + "/" + fileName;
	}

	ofstream myfile;
	myfile.open(filePath.data(), ios::out | ios::trunc | ios::binary);

	if (!myfile.good()) {
		std::cerr << "Unable to write to file " << filePath << std::endl;
		// carry on to free the memory. myfile.write will not throw!
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
	BKMFile.open(BKMFilePath.data(), ios::out | ios::trunc);

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
	std::cout << "Wrote BKM file " << BKMFilePath << std::endl;
}

std::string Merger::generateFileName(uint32_t runNumber, uint32_t burstID, uint32_t duplicate) {
	char buffer[64];

	if (duplicate == 0) {
		sprintf(buffer, "cdr%02d%06d-%04d.dat", Options::MERGER_ID, runNumber, burstID);
	} else {
		sprintf(buffer, "cdr%02d%06d-%04d_%d.dat", Options::MERGER_ID, runNumber, burstID, duplicate);
	}
	return std::string(buffer);
}

} /* namespace merger */
} /* namespace na62 */
