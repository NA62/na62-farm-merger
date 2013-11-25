/*
 * Options.h
 *
 *  Created on: Nov 9, 2011
 *      Author: kunzejo
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <iostream>
#include <stdint.h>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "../exceptions/BadOption.h"
#include "../exceptions/UnknownSourceIDFound.h"
#include "../utils/Utils.h"

namespace na62 {
namespace merger {
/*
 * Compile time options
 */
#define MTU 1500
#define LKR_SOURCE_ID 0x24

/*
 * Dynamic Options
 */
#define OPTION_HELP (char*)"help"
#define OPTION_VERBOSE (char*)"verbose"
#define OPTION_CONFIG_FILE (char*)"configFile"

#define OPTION_LISTEN_IP (char*)"listenIP"
#define OPTION_FIRST_LISTEN_PORT (char*)"firstListenPort"
#define OPTION_NUMBER_OF_PORTS (char*)"numberOfListenPorts"

#define OPTION_STORAGE_DIR (char*)"storageDir"
#define OPTION_BKM_DIR (char*)"bkmDir"

#define OPTION_DIM_UPDATE_TIME (char*)"dimUpdateTime"

#define OPTION_MERGER_ID (char*)"mergerID"

#define OPTION_RUN_NUMBER (char*)"currentRunNumber"

#define OPTION_TIMEOUT (char*)"burstTimeout"

class Options {
public:
	static void PrintVM(boost::program_options::variables_map vm);
	static void Initialize(int argc, char* argv[]);

	/*
	 * Configurable Variables
	 */
	static bool VERBOSE;

	static std::string LISTEN_IP;
	static uint FIRST_LISTEN_PORT;
	static uint NUMBER_OF_LISTEN_PORTS;

	static std::string STORAGE_DIR;
	static std::string BKM_DIR;

	static int MONITOR_UPDATE_TIME;

	static int MERGER_ID;

	static int RUN_NUMBER;

	static int TIMEOUT;

};
} /* namespace merger */
} /* namespace na62 */
#endif /* OPTIONS_H_ */
