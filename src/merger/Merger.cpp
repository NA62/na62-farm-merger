/*
 * Merger.cpp
 *
 *  Created on: Jul 6, 2012
 *      Author: Jonas Kunze <kunze.jonas@gmail.com>
 */

#include "Merger.hpp"

#include <fstream>
#include <iostream>
#include <structs/Event.h>
#include <thread>

#include <iomanip>
#include <glog/logging.h>
#include <boost/date_time.hpp>

#include <utils/Utils.h>
#include <storage/BurstFileWriter.h>
#include "../options/MyOptions.h"

namespace na62 {
namespace merger {

Merger::Merger() :
		eventsInLastBurst_(0), storageDirs_({Options::GetString(OPTION_STORAGE_DIR) + "/", Options::GetString(OPTION_STORAGE_DIR_2) + "/"}), lastSeenRunNumber_(0), threshold_(Options::GetInt(OPTION_DISk_THRESHOLD)) {

	    //Creating my mutex pool...
		for (uint32_t key = 1; key <= 5000; ++key) {
		    mutexbyBurstID_.emplace(key, new std::mutex);
		}

		//Starting EOB Collector thread
		EobThread_ = std::thread{&na62::merger::EobCollector::run, &eobCollector_};

		burstWritePool_.attachData(this);
		burstWritePool_.start(4);
}

Merger::~Merger() {
}

void Merger::push(zmq::message_t* event) {
	events_queue_.push(event);
}

void Merger::thread() {
	LOG_INFO("Starting Merger Thread");
	is_running_ = true;
	while (is_running_) {
		zmq::message_t * eventMessage = nullptr;
		events_queue_.try_pop(eventMessage);
		if (eventMessage == nullptr) {
			continue;
		}
		EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

		if (eventMessage->size() != event->length * 4) {
			LOG_ERROR("Received " << eventMessage->size() << " Bytes with an event of length " << (event->length * 4) );
			delete eventMessage;
			continue;
		}

		uint32_t burstID = event->burstID;
		{
			std::lock_guard<std::mutex> lock(*(mutexbyBurstID_[burstID]));

			std::map<uint32_t, zmq::message_t*>& burstMap = eventsByBurstByID_[burstID];
			if (burstMap.size() == 0) {
				LOG_INFO("New burst received submitting job: " << burstID);
				burstWritePool_.pushTask(burstID);
			}

			auto lb = burstMap.lower_bound(event->eventNum);

			if (lb != burstMap.end() && !(burstMap.key_comp()(event->eventNum, lb->first))) {
				LOG_ERROR("Event " << event->eventNum << " received twice. Dropping the second one!");
				delete eventMessage;
			} else {
				/*
				 * Event does not yet exist -> add it to the map
				 * As eventNum is a bitfield and value_type takes it's reference, we have to make it a rvalue->use +event->evenNum
				 */
				burstMap.insert(lb, std::map<uint32_t, zmq::message_t*>::value_type(+event->eventNum, eventMessage));
			}
		}
	}
}

void Merger::stopPool() {
	burstWritePool_.stop();
	eobCollector_.stop();
	is_running_ = false;
}

void Merger::onInterruption() {
	shutdown();
}

void Merger::shutdown() {
	LOG_INFO("Stopping merger queue pull");
	is_running_ = false;
	LOG_INFO("Stopping burstWritePool");
	burstWritePool_.stop();
	LOG_INFO("Stopping EobCollector");
	eobCollector_.stop();
	LOG_INFO("Joining EobCollector Thread");
	EobThread_.join();
}

} /* namespace merger */
} /* namespace na62 */
