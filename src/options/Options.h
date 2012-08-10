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
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include "../exceptions/BadOption.h"
#include "../exceptions/UnknownSourceIDFound.h"
#include "../messages/WarningHandler.h"
#include "../utils/Utils.h"

namespace na62 {
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
#define OPTION_LISTEN_PORT (char*)"listenPort"

#define OPTION_STORAGE_DIR (char*)"storageDir"

class Options {
public:
	static void PrintVM(boost::program_options::variables_map vm);
	static void Initialize(int argc, char* argv[]);


	/*
	 * Configurable Variables
	 */
	static bool VERBOSE;

	static std::string LISTEN_IP;
	static std::string LISTEN_PORT;

	static std::string STORAGE_DIR;

};
}
/* namespace na62 */
#endif /* OPTIONS_H_ */
