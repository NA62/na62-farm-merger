/*
 * AExecutable.h
 *
 *  Created on: Jul 22, 2012
 *      Author: root
 */

#ifndef AEXECUTABLE_H_
#define AEXECUTABLE_H_

#include <vector>

#include <boost/thread.hpp>

#include "../options/Options.h"
namespace na62 {
namespace merger {

class AExecutable {
public:
	AExecutable();
	virtual ~AExecutable();

	void startThread() {
		threadNum_ = 0;
		thread_ = boost::thread(boost::bind(&AExecutable::thread, this));
		threads_.add_thread(&thread_);
	}

	void startThread(unsigned short threadNum, std::vector<unsigned short> CPUMask, unsigned threadPrio) {
		threadNum_ = threadNum;
		thread_ = boost::thread(boost::bind(&AExecutable::thread, this));
		threads_.add_thread(&thread_);

		SetThreadAffinity(thread_, threadPrio, CPUMask, 0);
	}

	void startThread(unsigned short threadNum, unsigned short CPUMask, unsigned threadPrio) {
		threadNum_ = threadNum;
		thread_ = boost::thread(boost::bind(&AExecutable::thread, this));
		threads_.add_thread(&thread_);

		SetThreadAffinity(thread_, 15, CPUMask, 0);
	}

	static void SetThreadAffinity(boost::thread& daThread, unsigned short threadPriority, short unsigned CPUToBind, int scheduler);

	static void SetThreadAffinity(boost::thread& daThread, unsigned short threadPriority, std::vector<unsigned short> CPUsToBind, int scheduler);

	void join() {
		thread_.join();
	}

	void interrupt() {
		thread_.interrupt();
	}

	static void InterruptAll() {
		for (unsigned int i = 0; i < instances_.size(); i++) {
			instances_[i]->interrupt();
		}
//		threads_.interrupt_all();
	}

protected:
	short threadNum_;

private:
	virtual void thread() {
	}
	;

	boost::thread thread_;
	static boost::thread_group threads_;
	static std::vector<AExecutable*> instances_;
};

} /* namespace merger */
} /* namespace na62 */

#endif /* AEXECUTABLE_H_ */
