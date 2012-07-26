/*
 * Stopwatch.hpp
 *
 *  Created on: Apr 21, 2011
 *      Author: kunzejo
 */

#ifndef STOPWATCH_HPP_
#define STOPWATCH_HPP_

#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <stdint.h>

#include <string>
#include <boost/lexical_cast.hpp>
#include <stdio.h>
#include <vector>
#include <iostream>

#include <sys/resource.h>
#include <sys/types.h>

#include<map>

namespace na62 {
class Stopwatch {

private:
	struct rusage CPUStartTime;
	struct timeval RealStartTime;

	std::vector<double> realTimes;

public:
	Stopwatch() {
		start();
	}

	virtual ~Stopwatch() {

	}

	static inline uint64_t GetTicks() {
		unsigned a, d;
		asm volatile("rdtsc" : "=a" (a), "=d" (d));
		return ((unsigned long) a) | (((unsigned long) d) << 32);
	}

	/**
	 * Returns the CPU frequency in HZ
	 */
	static uint64_t cpuFrequency;
	static uint64_t ticksForOneUSleep;
	static inline uint64_t GetCPUFrequency() {
		if (cpuFrequency > 0) {
			return cpuFrequency;
		}
		return UpdateCPUFrequency();
	}

	/*
	 * Number of ticks that pass during a <usleep(1)>
	 */
	static inline uint64_t GetTicksForOneUSleep() {
		if (cpuFrequency > 0) {
			return ticksForOneUSleep;
		}
		UpdateCPUFrequency();
		return ticksForOneUSleep;
	}

	static inline uint64_t UpdateCPUFrequency() {
//		/*
//		 * Heat up CPU for turbo mode
//		 */
//		for (int i = 0; i < 10000; i++) {
//			rand();
//		}

// cumputing usleep delay
		uint64_t tick_start = GetTicks();
		usleep(1);
		ticksForOneUSleep = GetTicks() - tick_start;

		// cumputing CPU freq
		tick_start = GetTicks();
		usleep(1001);
		cpuFrequency = (GetTicks() - tick_start - ticksForOneUSleep) * 1000 /*kHz -> Hz*/;
//		std::cout << "Estimated CPU freq: " << cpuFrequency << " Hz\n" << std::endl;

		return cpuFrequency;
	}

	inline void start() {
		getrusage(RUSAGE_SELF, &CPUStartTime);
		gettimeofday(&RealStartTime, NULL);
	}

	inline void reset() {
		realTimes.clear();
		this->restart();
	}

	inline void restart() {
		this->start();
	}

	inline void stop() {
		this->realTimes.push_back(getRealTime());
	}

	inline double getUserCPUTime() {
		double sec, usec;
		struct rusage ruse;

		getrusage(RUSAGE_SELF, &ruse);

		sec = static_cast<double>(ruse.ru_utime.tv_sec);
		sec -= static_cast<double>(CPUStartTime.ru_utime.tv_sec);

		usec = static_cast<double>(ruse.ru_utime.tv_usec);
		usec -= static_cast<double>(CPUStartTime.ru_utime.tv_usec);
		return (sec + usec / 1E6);
	}

	inline double getCPULoad() {
		double sTime = getSystemCPUTime();
		double uTime = getUserCPUTime();
		double rTime = getRealTime();
		return (sTime + uTime) / rTime;
	}

	inline double getSystemCPUTime() {
		double sec, usec;
		struct rusage ruse;

		getrusage(RUSAGE_SELF, &ruse);

		sec = static_cast<double>(ruse.ru_stime.tv_sec);
		sec -= static_cast<double>(CPUStartTime.ru_stime.tv_sec);

		usec = static_cast<double>(ruse.ru_stime.tv_usec);
		usec -= static_cast<double>(CPUStartTime.ru_stime.tv_usec);
		return (sec + usec / 1E6);
	}

	static inline uint64_t GetCurrentMils(){
		struct timeval tp;

		gettimeofday(&tp, NULL);
		return tp.tv_sec*1000+tp.tv_usec/1000;
	}

	inline double getRealTime() {
		struct timeval tp;
		double sec, usec;

		gettimeofday(&tp, NULL);
		sec = static_cast<double>(tp.tv_sec);
		sec -= static_cast<double>(RealStartTime.tv_sec);
		usec = static_cast<double>(tp.tv_usec);
		usec -= static_cast<double>(RealStartTime.tv_usec);
		return sec + usec / 1E6;
	}

	inline __time_t getRealTimeSeconds() {
		struct timeval tp;
		gettimeofday(&tp, NULL);
		return tp.tv_sec - RealStartTime.tv_sec;
	}

	double getAverageRealTime() {
		double sum = 0;

		std::vector<double>::iterator itr;
		for (itr = realTimes.begin(); itr < realTimes.end(); ++itr) {
			sum += *itr;
		}
		return sum / realTimes.size();
	}

	double getStandardDevationOfAverageRealTime() {
		double av = getAverageRealTime();
		double sum = 0;

		for (int i = realTimes.size() - 1; i >= 0; i--) {
			sum += (realTimes[i] - av) * (realTimes[i] - av);
		}

		return sqrt(sum / (realTimes.size() - 1));
	}
};
}
#endif /* STOPWATCH_HPP_ */
