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
#include <pwd.h>
#include <iomanip>
#include <glog/logging.h>
#include <boost/date_time.hpp>

#include <utils/Utils.h>

#include "../options/MyOptions.h"

#include "Merger.hpp"

namespace na62 {
namespace merger {

Merger::Merger() :
		eventsInLastBurst_(0), currentRunNumber_(Options::GetInt(OPTION_RUN_NUMBER)), nextBurstSOBtimestamp_(0), storageDir_(
				Options::GetString(OPTION_STORAGE_DIR) + "/") {

	eobCollector_.run();

	struct passwd *pwd = getpwnam("na62cdr"); /* don't free, see getpwnam() for details */
	ownerID_ = pwd->pw_uid;
	groupID_ = pwd->pw_gid;

}

void Merger::addPacket(zmq::message_t* eventMessage) {
	EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

	if (currentRunNumber_ == 0) {
		LOG_INFO << "Run number not yet set -> Dropping incoming events";
		delete eventMessage;
		return;
	}

	uint32_t burstID = event->burstID;
	std::lock_guard<std::mutex> lock(eventMutex);

	std::map<uint32_t, zmq::message_t*>& burstMap = eventsByBurstByID[burstID];
	if (burstMap.size() == 0) {
		if (nextBurstSOBtimestamp_ == 0) { // didn't receive nextBurstID yet
			/*
			 * Give it another try (at startup it can take a while until we get the message from dim...
			 */
			usleep(1000);
			if (nextBurstSOBtimestamp_ == 0) {
				LOG_ERROR << "Received event even though the SOB is not defined yet. Dropping data!";
				delete eventMessage;
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

	auto lb = burstMap.lower_bound(event->eventNum);

	if (lb != burstMap.end() && !(burstMap.key_comp()(event->eventNum, lb->first))) {
		LOG_ERROR << "Event " << event->eventNum << " received twice. Dropping the second one!";
		delete eventMessage;
	} else {
		/*
		 * Event does not yet exist -> add it to the map
		 * As eventNum is a bitfield and value_type takes it's reference, we have to make it a rvalue->use +event->evenNum
		 */
		burstMap.insert(lb, std::map<uint32_t, zmq::message_t*>::value_type(+event->eventNum, eventMessage));
	}
}

void Merger::handle_newBurst(uint32_t newBurstID) {
	LOG_INFO << "New burst: " << newBurstID;
	boost::thread(boost::bind(&Merger::startBurstControlThread, this, newBurstID));
	runNumberByBurst[newBurstID] = currentRunNumber_;
}

void Merger::startBurstControlThread(uint32_t& burstID) {
	size_t lastEventNum = -1;
	do {
		lastEventNum = eventsByBurstByID[burstID].size();
		sleep(MyOptions::GetInt(OPTION_TIMEOUT));
	} while (eventsByBurstByID[burstID].size() > lastEventNum);
	LOG_INFO << "Finishing burst " << burstID << " : " << eventsByBurstByID[burstID].size() << " because of normal timeout.";
	handle_burstFinished(burstID);
}

void Merger::handle_burstFinished(uint32_t finishedBurstID) {
	std::lock_guard<std::mutex> lock(newBurstMutex);

	std::map<uint32_t, zmq::message_t*> &burst = eventsByBurstByID[finishedBurstID];

	if (Options::GetInt(OPTION_APPEND_DIM_EOB)) {
		zmq::message_t* oldEobEventMessage = (--burst.end())->second; // Take the last event as EOB event
		zmq::message_t* eobEventMessage = eobCollector_.addEobDataToEvent(oldEobEventMessage);

		EVENT_HDR* eobEvent = reinterpret_cast<EVENT_HDR*>(eobEventMessage->data());

		eventsByBurstByID[finishedBurstID][eobEvent->eventNum] = eobEventMessage;
	}

	saveBurst(burst, finishedBurstID);
	eventsInLastBurst_ = eventsByBurstByID[finishedBurstID].size();
	eventsByBurstByID.erase(finishedBurstID);
}

void Merger::saveBurst(std::map<uint32_t, zmq::message_t*>& eventByID, uint32_t& burstID) {
	uint32_t sob = SOBtimestampByBurst[burstID];
	uint32_t runNumber = runNumberByBurst[burstID];
	runNumberByBurst.erase(burstID);

	std::string fileName = generateFileName(sob, runNumber, burstID, 0);
	std::string filePath = storageDir_ + fileName;
	LOG_INFO << "Writing file " << filePath;

	int numberOfEvents = eventByID.size();
	if (numberOfEvents == 0) {
		LOG_ERROR << "No event received for burst " << burstID;
		return;
	}

	if (boost::filesystem::exists(filePath)) {
		LOG_ERROR << "File already exists: " << filePath;
		int counter = 2;
		fileName = generateFileName(sob, runNumber, burstID, counter);

		LOG_INFO << runNumber << "\t" << burstID << "\t" << counter << "\t" << fileName << "!!!";
		while (boost::filesystem::exists(storageDir_ + fileName)) {
			LOG_ERROR << "File already exists: " << fileName;
			fileName = generateFileName(sob, runNumber, burstID, ++counter);
			LOG_INFO << runNumber << "\t" << burstID << "\t" << counter << "\t" << fileName << "!!!";
		}

		LOG_ERROR << "Instead writing file: " << fileName;
		filePath = storageDir_ + fileName;
	}

	std::ofstream myfile;
	myfile.open(filePath.data(), std::ios::out | std::ios::trunc | std::ios::binary);

	if (!myfile.good()) {
		LOG_ERROR << "Unable to write to file " << filePath;
		// carry on to free the memory. myfile.write will not throw!
	}

	boost::posix_time::ptime start(boost::posix_time::microsec_clock::local_time());

	size_t bytes = 0;
	for (auto pair : eventByID) {
		myfile.write((char*) pair.second->data(), pair.second->size());
		bytes += pair.second->size();

		delete pair.second;
	}
	myfile.close();

	boost::posix_time::ptime stop(boost::posix_time::microsec_clock::local_time());
	boost::posix_time::time_duration dt = stop - start;
	long msec = dt.total_milliseconds();
	long dataRate = 0;
	if (msec != 0) {
		dataRate = bytes / msec * 1000; // B/s
	}

//	std::stringstream chownCommand;
//	chownCommand << "chown na62cdr:vl " << filePath;
//	system(chownCommand.str().c_str());
	chown(filePath.c_str(), ownerID_, groupID_);

	writeBKMFile(filePath, fileName, bytes);

	LOG_INFO << "Wrote burst " << burstID << " with " << numberOfEvents << " events and " << Utils::FormatSize(bytes) << "B with "
			<< Utils::FormatSize(dataRate) << "B/s";
}

void Merger::writeBKMFile(std::string dataFilePath, std::string fileName, size_t fileLength) {
	time_t rawtime;
	struct tm * timeinfo;
	char timeString[24];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(timeString, 64, "%d-%m-%y_%H:%M:%S", timeinfo);

	std::string BKMFilePath = Options::GetString(OPTION_BKM_DIR) + "/" + fileName;

	std::ofstream BKMFile;
	BKMFile.open(BKMFilePath.data(), std::ios::out | std::ios::trunc);

	if (!BKMFile.good()) {
		LOG_ERROR << "Unable to write to file " << BKMFilePath;
		return;
	}

	BKMFile.write(dataFilePath.data(), dataFilePath.length());
	BKMFile.write("\n", 1);

	std::string sizeLine = "size: " + std::to_string(fileLength);
	BKMFile.write(sizeLine.data(), sizeLine.length());
	BKMFile.write("\n", 1);

	std::string dateLine = "datetime: " + std::string(timeString);
	BKMFile.write(dateLine.data(), dateLine.length());
	BKMFile.write("\n", 1);

	BKMFile.close();

	system(std::string("chown na62cdr:vl " + BKMFilePath).data());

	LOG_INFO << "Wrote BKM file " << BKMFilePath;
}

std::string Merger::generateFileName(uint32_t sob, uint32_t runNumber, uint32_t burstID, uint32_t duplicate) {
	std::stringstream fileName;
	fileName << "na62raw_" << sob << "-" << std::setfill('0') << std::setw(2) << Options::GetInt(OPTION_MERGER_ID);
	fileName << "-" << std::setfill('0') << std::setw(6) << runNumber << "-";
	fileName << std::setfill('0') << std::setw(4) << burstID;

	if (duplicate != 0) {
		fileName << "_" << duplicate;
	}
	fileName << ".dat";
	return fileName.str();
}

} /* namespace merger */
} /* namespace na62 */
