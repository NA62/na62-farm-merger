/*
 * AExecutable2.cpp
 *
 *  Created on: Jul 22, 2012
 *      Author: root
 */

#include "../exceptions/NA62Error.h"
#include "AExecutable.h"

namespace na62 {
namespace merger {
boost::thread_group AExecutable::threads_;
std::vector<AExecutable*> AExecutable::instances_;

AExecutable::AExecutable(): threadNum_(-1) {
	instances_.push_back(this);
}

AExecutable::~AExecutable() {

}

void AExecutable::SetThreadAffinity(boost::thread& daThread, unsigned short threadPriority, short unsigned CPUToBind, int scheduler) {
	std::vector<unsigned short> CPUsToBind;
	CPUsToBind.push_back(CPUToBind);
	SetThreadAffinity(daThread, threadPriority, CPUsToBind, scheduler);
}

void AExecutable::SetThreadAffinity(boost::thread& daThread, unsigned short threadPriority, std::vector<unsigned short> CPUsToBind, int scheduler) {
	int policy;
	pthread_t threadID = (pthread_t) (daThread.native_handle());
	if (scheduler > 0) {

		struct sched_param param;
		if (pthread_getschedparam(threadID, &policy, &param) != 0) {
			perror("pthread_getschedparam");
			exit(EXIT_FAILURE);
		}

		/**
		 * Set scheduling algorithm
		 * Possible values: SCHED_FIFO, SCHED_RR, SCHED_OTHER
		 */
		policy = scheduler;
		param.sched_priority = threadPriority;
		if (pthread_setschedparam(threadID, policy, &param) != 0) {
			perror("pthread_setschedparam");
			exit(EXIT_FAILURE);
		}
	}

	if (CPUsToBind.size() > 0) {
		/**
		 * Bind the thread to CPUs from CPUsToBind
		 */
		cpu_set_t mask;
		CPU_ZERO(&mask);

		for (unsigned int i = 0; i < CPUsToBind.size(); i++) {
			if (CPUsToBind[i] == -1) {
				CPU_ZERO(&mask);
				break;
			}
			CPU_SET(CPUsToBind[i], &mask);
		}

		if (pthread_setaffinity_np(threadID, sizeof(mask), &mask) < 0) {
			throw NA62Error("Unable to bind threads to specific CPUs!");
		}
	}
}
} /* namespace merger */
} /* namespace na62 */
