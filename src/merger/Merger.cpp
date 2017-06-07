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
		storageDir_(Options::GetString(OPTION_STORAGE_DIR) + "/"),
		eventsInLastBurst_(0)
	{
	//boost::thread(boost::bind(&eobCollector_.run()), this);
	boost::thread(&na62::merger::EobCollector::run, &eobCollector_);
}

void Merger::addPacket(zmq::message_t* eventMessage) {
	EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

	uint32_t burstID = event->burstID;
	BurstTimeInfo bi;
	try {
		bi = eobCollector_.getBurstInfoByBurst(burstID);
	} catch (int e) {
		return;
	}

	//uint32_t SOBtimestamp = event->SOBtimestamp;
	uint32_t SOBtimestamp = bi.sobTime;
//	if (SOBtimestamp == 0) {
//		LOG_ERROR("Received a 0 timestamp packet");
//		return;
//	}
	std::lock_guard<std::mutex> lock(eventMutex);

	//std::map<uint32_t, zmq::message_t*>& burstMap = eventsByBurstByID[burstID];
	std::map<uint32_t, zmq::message_t*>& SOBtimestampMap = eventsBySobTsByID[SOBtimestamp];
	//if (burstMap.size() == 0) {
	if (SOBtimestampMap.size() == 0) {
		/*
		 * Only one thread will start the EOB checking thread
		 */
		if (newBurstMutex.try_lock()) {
			//handle_newBurst(burstID);
			handle_newBurst(SOBtimestamp);
			newBurstMutex.unlock();
		}
	}

	//auto lb = burstMap.lower_bound(event->eventNum);
	auto lb = SOBtimestampMap.lower_bound(event->eventNum);

	//if (lb != burstMap.end() && !(burstMap.key_comp()(event->eventNum, lb->first))) {
	if (lb != SOBtimestampMap.end() && !(SOBtimestampMap.key_comp()(event->eventNum, lb->first))) {
		LOG_ERROR("Event " << event->eventNum << " received twice. Dropping the second one!");
		delete eventMessage;
	} else {
		/*
		 * Event does not yet exist -> add it to the map
		 * As eventNum is a bitfield and value_type takes it's reference, we have to make it a rvalue->use +event->evenNum
		 */
		SOBtimestampMap.insert(lb, std::map<uint32_t, zmq::message_t*>::value_type(+event->eventNum, eventMessage));
	}
}

//void Merger::handle_newBurst(uint32_t newBurstID) {
void Merger::handle_newBurst(uint32_t SOBtimestamp) {
	//LOG_INFO("New burst: " << newBurstID);
	LOG_INFO("New burst with SOB timestamp: " << SOBtimestamp);
	//boost::thread(boost::bind(&Merger::startBurstControlThread, this, newBurstID));
	boost::thread(boost::bind(&Merger::startBurstControlThread, this, SOBtimestamp));
}

//void Merger::startBurstControlThread(uint32_t& burstID) {
void Merger::startBurstControlThread(uint32_t& SOBtimestamp) {
	size_t lastEventNum = -1;

	do {
		//lastEventNum = eventsByBurstByID[burstID].size();
		lastEventNum = eventsBySobTsByID[SOBtimestamp].size();
		sleep(MyOptions::GetInt(OPTION_TIMEOUT));
	//} while (eventsByBurstByID[burstID].size() > lastEventNum);
	} while (eventsBySobTsByID[SOBtimestamp].size() > lastEventNum);

	//LOG_INFO("Finishing burst " << burstID << " : " << eventsByBurstByID[burstID].size() << " because of normal timeout.");
	LOG_INFO("Finishing burst with SOB timestamp " << SOBtimestamp << " : " << eventsBySobTsByID[SOBtimestamp].size() << " because of normal timeout.");
	//handle_burstFinished(burstID);
	handle_burstFinished(SOBtimestamp);
}

//void Merger::handle_burstFinished(uint32_t finishedBurstID) {
void Merger::handle_burstFinished(uint32_t finishedSOBtimestamp) {
	std::lock_guard<std::mutex> lock(newBurstMutex);
	//std::map<uint32_t, zmq::message_t*> &burst = eventsByBurstByID[finishedBurstID];
	std::map<uint32_t, zmq::message_t*> &burst = eventsBySobTsByID[finishedSOBtimestamp];
	//uint32_t sob = 0;
	uint32_t burstID = 0;
	uint32_t runNumber = 0;
	try {
		try {
			//BurstTimeInfo burstInfo = eobCollector_.getBurstInfoSOB(finishedBurstID);
			BurstTimeInfo burstInfo = eobCollector_.getBurstInfoSOB(finishedSOBtimestamp);
			runNumber = burstInfo.runNumber;
			burstID = burstInfo.burstID;
			//sob = burstInfo.sobTime;
		} catch (std::exception &e) {
			BurstTimeInfo burstInfo = eobCollector_.getBurstInfoEOB(finishedSOBtimestamp);
			runNumber = burstInfo.runNumber;
			burstID = burstInfo.burstID;
		}

		if (burstID == 0) {
			burstID = reinterpret_cast<EVENT_HDR*> ( ((zmq::message_t*) burst.begin()->second)->data())->burstID;
		}
		//Extend the EOB event with extra info from DIM
		if (Options::GetInt(OPTION_APPEND_DIM_EOB)) {
			zmq::message_t* oldEobEventMessage = (--burst.end())->second; // Take the last event as EOB event
			zmq::message_t* eobEventMessage = eobCollector_.addEobDataToEvent(oldEobEventMessage);

			EVENT_HDR* eobEvent = reinterpret_cast<EVENT_HDR*>(eobEventMessage->data());

			//eventsByBurstByID[finishedBurstID][eobEvent->eventNum] = eobEventMessage;
			eventsBySobTsByID[finishedSOBtimestamp][eobEvent->eventNum] = eobEventMessage;
		}

	} catch (std::exception &e) {
		LOG_ERROR("Missing DIM information for burst with SOB timestamp " << finishedSOBtimestamp
				<< " in run " << runNumber << ". This burst will have missing SOB/EOB information!");
	}

	//saveBurst(burst, lastSeenRunNumber_, finishedBurstID, sob);
	saveBurst(burst, runNumber, burstID, finishedSOBtimestamp);
	//eventsInLastBurst_ = eventsByBurstByID[finishedBurstID].size();
	LOG_INFO("Fetching map size for display before delete");
	eventsInLastBurst_ = eventsBySobTsByID[finishedSOBtimestamp].size();
	//eventsByBurstByID.erase(finishedBurstID);
	LOG_INFO("Erasing entry from the map: " << finishedSOBtimestamp);
	eventsBySobTsByID.erase(finishedSOBtimestamp);
}

void Merger::saveBurst(std::map<uint32_t, zmq::message_t*>& eventByID, uint32_t& runNumber, uint32_t& burstID, uint32_t& sobTime) {
	std::string fileName = generateFileName(sobTime, runNumber, burstID, 0);
	std::string filePath = storageDir_ + fileName;
	LOG_INFO("Writing file " << filePath);

	int numberOfEvents = eventByID.size();
	if (numberOfEvents == 0) {
		LOG_ERROR("No event received for burst " << burstID);
		return;
	}

	if (boost::filesystem::exists(filePath)) {
		LOG_ERROR("File already exists: " << filePath);
		int counter = 2;
		fileName = generateFileName(sobTime, runNumber, burstID, counter);

		LOG_INFO(runNumber << "\t" << burstID << "\t" << counter << "\t" << fileName << "!!!");
		while (boost::filesystem::exists(storageDir_ + fileName)) {
			LOG_ERROR("File already exists: " << fileName);
			fileName = generateFileName(sobTime, runNumber, burstID, ++counter);
			LOG_INFO(runNumber << "\t" << burstID << "\t" << counter << "\t" << fileName << "!!!");
		}

		LOG_ERROR("Instead writing file: " << fileName);
		filePath = storageDir_ + fileName;
	}

	BurstFileWriter writer(filePath, fileName, eventByID.size(), sobTime, runNumber, burstID);
	LOG_INFO("Writing data from the map for timestamp:" << sobTime << " Burst: " << burstID);
	for (auto pair : eventByID) {
		EVENT_HDR* ev = reinterpret_cast<EVENT_HDR*>(pair.second->data());
		// Set SOB time in the event
		ev->SOBtimestamp = sobTime;
		writer.writeEvent(ev);
		delete pair.second;
	}
	LOG_INFO("File completed" << filePath);
	//writer.writeBkmFile(Options::GetString(OPTION_BKM_DIR));
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
