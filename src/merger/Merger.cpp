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

#include <iomanip>
#include <glog/logging.h>
#include <boost/date_time.hpp>

#include <utils/Utils.h>

#include <storage/BurstFileWriter.h>

#include "../options/MyOptions.h"

#include "Merger.hpp"

namespace na62 {
namespace merger {

Merger::Merger() :
		eventsInLastBurst_(0), currentRunNumber_(Options::GetInt(OPTION_RUN_NUMBER)), nextBurstSOBtimestamp_(0), storageDir_(
				Options::GetString(OPTION_STORAGE_DIR) + "/") {

	eobCollector_.run();

}

void Merger::addPacket(zmq::message_t* eventMessage) {
	EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

	if (currentRunNumber_ == 0) {
		LOG_INFO<< "Run number not yet set -> Dropping incoming events";
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
				LOG_ERROR<< "Received event even though the SOB is not defined yet. Dropping data!";
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
		LOG_ERROR<< "Event " << event->eventNum << " received twice. Dropping the second one!";
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
	LOG_INFO<< "New burst: " << newBurstID;
	boost::thread(boost::bind(&Merger::startBurstControlThread, this, newBurstID));
	runNumberByBurst[newBurstID] = currentRunNumber_;
}

void Merger::startBurstControlThread(uint32_t& burstID) {
	size_t lastEventNum = -1;
	do {
		lastEventNum = eventsByBurstByID[burstID].size();
		sleep(MyOptions::GetInt(OPTION_TIMEOUT));
	} while (eventsByBurstByID[burstID].size() > lastEventNum);
	LOG_INFO<< "Finishing burst " << burstID << " : " << eventsByBurstByID[burstID].size() << " because of normal timeout.";
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
	LOG_INFO<< "Writing file " << filePath;

	int numberOfEvents = eventByID.size();
	if (numberOfEvents == 0) {
		LOG_ERROR<< "No event received for burst " << burstID;
		return;
	}

	if (boost::filesystem::exists(filePath)) {
		LOG_ERROR<< "File already exists: " << filePath;
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

	BurstFileWriter writer(filePath, fileName, eventByID.size(), sob, runNumber, burstID);

	for (auto pair : eventByID) {
		writer.writeEvent(reinterpret_cast<EVENT_HDR*>(pair.second->data()));

		delete pair.second;
	}

	writer.writeBkmFile(Options::GetString(OPTION_BKM_DIR));
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
