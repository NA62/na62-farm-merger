/*
 * MyOptions.h
 *
 *  Created on: Apr 11, 2014
 \*      Author: Jonas Kunze (kunze.jonas@gmail.com)
 */

#pragma once
#ifndef MYOPTIONS_H_
#define MYOPTIONS_H_

#include <options/Options.h>
#include <string>
#include <boost/thread.hpp>

/*
 * Dynamic Options
 */
#define OPTION_LISTEN_PORT (char*)"listenPort"

#define OPTION_STORAGE_DIR (char*)"storageDir"
#define OPTION_BKM_DIR (char*)"bkmDir"

#define OPTION_THREAD_NUM (char*)"numberOfWorkerThreads"

#define OPTION_DIM_UPDATE_TIME (char*)"dimUpdateTime"

#define OPTION_MERGER_ID (char*)"mergerID"

#define OPTION_RUN_NUMBER (char*)"currentRunNumber"

#define OPTION_TIMEOUT (char*)"burstTimeout"

#define OPTION_EOB_COLLECTION_TIMEOUT (char*)"eobCollectionTimeout"

#define OPTION_APPEND_DIM_EOB (char*)"appendDimEob"

namespace na62 {
class MyOptions: public Options {
public:
	MyOptions();
	virtual ~MyOptions();

	static void Load(int argc, char* argv[]) {
		desc.add_options()

		(OPTION_CONFIG_FILE, po::value<std::string>()->default_value("/etc/merger.conf"), "Config file for these options")

		(OPTION_LISTEN_PORT, po::value<int>()->required(), "tcp-port the merger should listen to")

		(OPTION_STORAGE_DIR, po::value<std::string>()->required(), "Path to the directory where burst files should be written to.")

		(OPTION_BKM_DIR, po::value<std::string>()->required(),
				"Path to the directory where the bkm files with the path to the original burst file and some statistics should be written after finishing each burst.")

		(OPTION_THREAD_NUM, po::value<int>()->default_value(0), "Number of worker threads. If it is 0 the number of CPU cores will be used")

		(OPTION_DIM_UPDATE_TIME, po::value<int>()->required(), "Milliseconds to sleep between two monitor updates.")

		(OPTION_MERGER_ID, po::value<int>()->required(), "Identifier of this merger. This will be the first number of the created files.")

		(OPTION_RUN_NUMBER, po::value<int>()->default_value(0),
				"Current run number (will be updated via dim). Set to 0 if you want to wait for the dim info before starting")

		(OPTION_TIMEOUT, po::value<int>()->required(),
				"If we didn't receive <burstTimeout> seconds any event from one burst the file will be written.")

		(OPTION_EOB_COLLECTION_TIMEOUT, po::value<int>()->default_value(500), "Wait this many ms after an EOB before reading the EOB services")

		(OPTION_APPEND_DIM_EOB, po::value<int>()->default_value(1), "Set to 0 if no dim EOB data should be appended to the EOB events")

		;

		Options::Initialize(argc, argv, desc);
	}
};

} /* namespace na62 */

#endif /* MYOPTIONS_H_ */
