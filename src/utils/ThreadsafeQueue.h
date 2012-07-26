/*
 * ThreadsafeQueue.h
 *
 * This class is based on the proposal at following blog post:
 * na62://msmvps.com/blogs/vandooren/archive/2007/01/05/creating-a-thread-safe-producer-consumer-queue-in-c-without-using-locks.aspx
 *
 * A thread safe consumer-producer queue. This means this queue is only thread safe if you have only one writer-thread and only one reader-thread.
 *  Created on: Jan 5, 2012
 *      Author: root
 */

#ifndef THREADSAFEQUEUE_H_
#define THREADSAFEQUEUE_H_

#include <boost/thread.hpp>

namespace na62 {

template<class T> class ThreadsafeQueue {
private:
//	volatile int eadPos_;
//	volatile int writePos_;
//	static const int Size_ = 100000;
//	volatile T Data[Size_];

	volatile uint32_t readPos_;
	volatile uint32_t writePos_;
	const uint32_t Size_;
	T* Data;

public:
	ThreadsafeQueue(uint32_t size) :
			Size_(size) {
		readPos_ = 0;
		writePos_ = 0;
		Data = new T[Size_];
	}

	~ThreadsafeQueue() {
		delete[] Data;
	}

	/*
	 * Push a new element into the circular queue. May only be called by one single thread (producer)!
	 */
	bool push(T &element) throw () {
		const uint32_t nextElement = (writePos_ + 1) % Size_;
		if (nextElement != readPos_) {
			Data[writePos_] = element;
			writePos_ = nextElement;
			return true;
		}
		return false;
	}

	/*
	 * remove the oledest element from the circular queue. May only be called by one single thread (consumer)!
	 */
	bool pop(T &element) throw () {
		if (readPos_ == writePos_) {
			return false;
		}
		const uint32_t nextElement = (readPos_ + 1) % Size_;

		element = Data[readPos_];
		readPos_ = nextElement;
		return true;
	}

	uint32_t size() {
		return Size_;
	}

	uint32_t getCurrentLength() {
		if(readPos_ <= writePos_){
			return writePos_ - readPos_;
		} else {
			return Size_ - readPos_ + writePos_;
		}

	}
};

} /* namespace na62 */
#endif /* THREADSAFEQUEUE_H_ */
