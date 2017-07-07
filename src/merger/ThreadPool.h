/*
 * Merger.h
 *
 *  Created on: Jul 5, 2017
 *      Author: Marco Boretto (marco.bore@gmail.com)
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_


#include <vector>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <sstream>


namespace na62 {
namespace merger {

class Merger;

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();
    void start(uint pool_size);
    void stop();
    void pushTask(uint task);

    void attachData(Merger* merger_data) {
    	merger_data_ = merger_data;
    }


	std::string getProgressStats() {
		std::unique_lock<std::mutex> lock(status_access_);
		std::stringstream stream;
		for (auto pair : pool_burst_progress_) {
			if (pair.first == 0) {
				continue;
			}
			stream << pair.first << ":" << pair.second << ";";
		}
		lock.unlock();
		return stream.str();
	}

	uint getNumberOfEventsInLastBurst(){
		return event_in_last_burst_;
	}


private:
    uint pool_size_;
    std::vector<std::thread> threads_;
    std::atomic<bool> is_running_;
    std::condition_variable new_job_;
    std::mutex queue_access_;
    std::deque<uint> queue_;
    void drowsiness(uint id);
    void doTask(uint thread_id, uint mytask);


    std::vector<std::pair<uint,uint>> pool_burst_progress_;
    uint event_in_last_burst_; //Last burst written by the pool hopefully the most recent one
    std::mutex status_access_;


    Merger* merger_data_;
    void handleBurstFinished(uint thread_id, uint32_t finishedBurstID);


    void saveBurst(uint thread_id, uint32_t runNumber, uint32_t burstID, uint32_t sobTime);
    std::string generateFileName(uint32_t sob, uint32_t runNumber, uint32_t burstID, uint32_t duplicate);


};

}
}
#endif /* THREADPOOL_H_ */
