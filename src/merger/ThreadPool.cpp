#include "ThreadPool.h"

#include "Merger.hpp"
#include "../options/MyOptions.h"
#include <storage/BurstFileWriter.h>
#include <boost/filesystem.hpp>

namespace na62 {
namespace merger {

ThreadPool::ThreadPool() : is_running_(false) {
}

void ThreadPool::start(uint pool_size) {
	pool_size_ = pool_size;
	is_running_ = true;
    for (uint id = 0; id < pool_size_; ++id) {
        threads_.push_back(std::thread(&ThreadPool::drowsiness, this, id));
    }

    //std::unique_lock<std::mutex> lock(status_access_);
    for (uint id = 0; id < pool_size_; ++id) {
    	pool_burst_progress_.push_back({0, 0});
    }
    //lock.unlock();
}

void ThreadPool::pushTask(uint task) {
    std::lock_guard<std::mutex> lock(queue_access_);
    queue_.push_front(task);
    new_job_.notify_one();
}

void ThreadPool::drowsiness(uint thread_id) {
	LOG_INFO("Thread Pool Start thread: "<< thread_id);
    while(is_running_) {
        //try to take the lock on the queue and the let the thread go to sleep
        //Please notice that the lock is released after the sleep
        //when the thread will be awaken by the condition variable it will retake the lock
        std::unique_lock<std::mutex> lock(queue_access_);
        new_job_.wait_for(lock, std::chrono::seconds(6));//Will automatically check for a new job every  
        if (queue_.empty()) {
            //Spurious awake
            lock.unlock();
            continue;
        }

        uint my_task = queue_.back();
        queue_.pop_back();
        lock.unlock();
        LOG_INFO("ThreadWritePool id: " << thread_id <<" my burst is: "<< my_task);
        doTask(thread_id, my_task);
        LOG_INFO("ThreadWritePool id: " << thread_id <<" completed burst: "<< my_task);
    }
    LOG_INFO("Thread Pool Stopping thread: "<< thread_id);
}

void ThreadPool::doTask(uint thread_id, uint my_task) {
        int lastEventNum = -1;
        LOG_INFO("Burst: " << my_task << " Waiting for all the fragments.");
        while (true) {
			std::unique_lock<std::mutex> lock(merger_data_->getMutex(my_task));
			std::map<uint32_t, zmq::message_t*>& burstMap = merger_data_->accessTo(my_task);
			int currentEventNum = burstMap.size();
			lock.unlock();

			if (currentEventNum > lastEventNum) {
				lastEventNum = currentEventNum;
				LOG_INFO("Burst: " << my_task << " Timeout Started: " << MyOptions::GetInt(OPTION_TIMEOUT) << "s" );
				sleep(MyOptions::GetInt(OPTION_TIMEOUT));
				LOG_INFO("Burst: " << my_task << " Timeout Ended");
				continue;
			} else if (currentEventNum == lastEventNum) {
				break;
			} else {
				LOG_ERROR("Burst: " << my_task << " Inconsistency between number of received events and events stored on the map! currentEventNumber: " << currentEventNum << " lastEventNum: " << lastEventNum);
				break;
			}
        }

		LOG_INFO("Finishing burst " << my_task << " : " << lastEventNum << " because of normal timeout");

		std::unique_lock<std::mutex> lock(merger_data_->getMutex(my_task));
		handleBurstFinished(thread_id,my_task);
        lock.unlock();
}

void ThreadPool::handleBurstFinished(uint thread_id, uint32_t finishedBurstID) {
	uint32_t sob = 0;
	uint32_t lastSeenRunNumber = 0;
	try {
		BurstTimeInfo burstInfo = merger_data_->getEOBCollector().getBurstInfoSOB(finishedBurstID);
		lastSeenRunNumber = burstInfo.runNumber;
		sob = burstInfo.sobTime;
	} catch (std::exception &e) {
		LOG_ERROR("Missing DIM information for burst " << finishedBurstID
					<< " in run " <<  lastSeenRunNumber<< ". This burst will have missing SOB/EOB information!");
	} catch (...) {
		LOG_ERROR("Unknown exception raised from EOB collector during burst" << finishedBurstID << " Run number: " << lastSeenRunNumber);
	}

	saveBurst(thread_id, lastSeenRunNumber, finishedBurstID, sob);
	merger_data_->erase(finishedBurstID);
}

void ThreadPool::saveBurst(uint thread_id, uint32_t runNumber, uint32_t burstID, uint32_t sobTime) {
	std::string fileName = generateFileName(sobTime, runNumber, burstID, 0);
	std::string filePath = merger_data_->getStorageDir() + fileName;
	LOG_INFO("Writing file " << filePath);

	std::map<uint32_t, zmq::message_t*> eventByID = merger_data_->accessTo(burstID);

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
		while (boost::filesystem::exists(merger_data_->getStorageDir() + fileName)) {
			LOG_ERROR("File already exists: " << fileName);
			fileName = generateFileName(sobTime, runNumber, burstID, ++counter);
			LOG_ERROR(runNumber << "\t" << burstID << "\t" << counter << "\t" << fileName << "!!!");
		}

		LOG_ERROR("Instead writing file: " << fileName);
		filePath = merger_data_->getStorageDir() + fileName;
	}

	BurstFileWriter writer(filePath, fileName, eventByID.size(), sobTime, runNumber, burstID);
	LOG_INFO("Starting write: " << fileName);

	uint amount = 0;
	for (auto pair = eventByID.begin(); pair != eventByID.end(); ++pair) {
		bool last_iteration = pair == (--eventByID.end());

		try {
			if (pair->second->data() == nullptr) {
				LOG_ERROR("No ZMQ packet found...");
				continue;
			}
			EVENT_HDR* ev = reinterpret_cast<EVENT_HDR*>(pair->second->data());

			if (pair->second->size() < sizeof(EVENT_HDR)) {
				LOG_ERROR("ZMQ Packet smaller than the event header...");
				delete pair->second;
				continue;
			}

			//Updating statistics
			amount++;
			if ((amount % 50) == 0) {
				//std::unique_lock<std::mutex> lock(status_access_);
				pool_burst_progress_[thread_id].first = burstID;
				pool_burst_progress_[thread_id].second = amount;
				//lock.unlock();
			}

			//Check if it is a real EOB
			if (ev->getL0TriggerTypeWord() == TRIGGER_L0_EOB) {
				if (!last_iteration) {
					LOG_ERROR("The EOB event " << burstID << " in run " <<  runNumber << " is not the last event of the burst");
				}
				//Extend the EOB event with extra info from DIM
				if (Options::GetInt(OPTION_APPEND_DIM_EOB)) {
					LOG_INFO("Processing last event of burst...");
					try {
						char* eobEventMessage = merger_data_->getEOBCollector().addEobDataToEvent(pair->second);
						ev = reinterpret_cast<EVENT_HDR*>(eobEventMessage);
						// Set SOB time in the event
						ev->SOBtimestamp = sobTime;
						writer.writeEvent(ev);
						delete ev;
						continue; //To avoid the execution of the section below
						//I could use alse break since is the last event of bust
					} catch (std::exception &e) {
						LOG_ERROR("Missing DIM information for burst " << burstID
									<< " in run " <<  runNumber<< ". This burst will have missing SOB/EOB information!");
					} catch (...) {
						 LOG_ERROR("Unknown exception raised fetching data from EOBCollector burst: " << burstID << " in run " <<  runNumber);
					}
				}
			}

			// Set SOB time in the event
			ev->SOBtimestamp = sobTime;
			writer.writeEvent(ev);
			delete pair->second;

		} catch(const zmq::error_t& ze) {
		   LOG_ERROR("Exception: " << ze.what()  << " burst " << burstID << " in run " <<  runNumber );
		} catch (...) {
			 LOG_ERROR("Unknown exception raised from zmq burst: " << burstID << " in run " <<  runNumber);
		}
	}
	//Updating to the maximum value
	{
		//std::unique_lock<std::mutex> lock(status_access_);
		pool_burst_progress_[thread_id].second = amount;
		//lock.unlock();
	}

	LOG_INFO("Ended write burst: " << burstID << " in run " <<  runNumber << " filename " << fileName);

	try {
		if (writer.doChown(filePath, "na62cdr", "root")) {
			LOG_INFO("Correctly changed permissions to: " << fileName);
		} else {
			LOG_ERROR("Chown failed on file: " << fileName);
		}
	} catch (const std::exception & e) {
		LOG_ERROR("Chown Failed " << e.what() << "filename" << fileName );
	}

	sleep(2); //Let fetch the latest statistics
	{
		//std::unique_lock<std::mutex> lock(status_access_);
		event_in_last_burst_ = pool_burst_progress_[thread_id].second;
		pool_burst_progress_[thread_id].first = 0;
		pool_burst_progress_[thread_id].second = 0;
		//lock.unlock();
	}
}

std::string ThreadPool::generateFileName(uint32_t sob, uint32_t runNumber, uint32_t burstID, uint32_t duplicate) {
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

void ThreadPool::stop() {
	LOG_INFO("Stopping write Pool");
    
    while (true) {
        std::unique_lock<std::mutex> lock(queue_access_);
        if (!queue_.empty()) {
            lock.unlock();
            sleep(1);
            LOG_INFO("Flushing the jobs queue.. ");
            new_job_.notify_all();
            continue;
        }
        break;
    }
    is_running_ = false;
    new_job_.notify_all();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
        	LOG_INFO("Thread: "<< thread.get_id());
            thread.join();
        }
    }
}

ThreadPool::~ThreadPool() {
}

}
}
