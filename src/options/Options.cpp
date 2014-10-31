/*
 * Options.cpp
 *
 *  Created on: Nov 9, 2011
 *      Author: kunzejo
 */

#include<boost/filesystem.hpp>
#include "Options.h"

namespace na62 {
namespace merger {

namespace po = boost::program_options;

/*
 * Configurable Variables
 */
bool Options::VERBOSE;
uint Options::LISTEN_PORT;
std::string Options::STORAGE_DIR;
std::string Options::BKM_DIR;
int Options::THREAD_NUM;

int Options::MONITOR_UPDATE_TIME;
int Options::MERGER_ID;
int Options::RUN_NUMBER;
int Options::TIMEOUT;
int Options::EOB_COLLECTION_TIMEOUT;
bool Options::APPEND_DIM_EOB;

void Options::PrintVM(po::variables_map vm) {
	using namespace po;
	for (variables_map::iterator it = vm.begin(); it != vm.end(); ++it) {
		std::cout << it->first << "=";

		const variable_value& v = it->second;
		if (!v.empty()) {
			const std::type_info& type = v.value().type();
			if (type == typeid(::std::string)) {
				std::cout << v.as<std::string>();
			} else if (type == typeid(int)) {
				std::cout << v.as<int>();
			}
		}
		std::cout << std::endl;
	}
}
/**
 * The constructor must be public but should not be called! Use Instance() as factory Method instead.
 */
void Options::Initialize(int argc, char* argv[]) {
	po::options_description desc("Allowed options");
	desc.add_options()(OPTION_HELP, "Produce help message")

	(OPTION_VERBOSE, "Verbose mode")

	(OPTION_CONFIG_FILE, po::value<std::string>()->default_value("/etc/merger.conf"), "Config file for these options")

	(OPTION_LISTEN_PORT, po::value<int>()->required(), "tcp-port the merger should listen to")

	(OPTION_STORAGE_DIR, po::value<std::string>()->required(), "Path to the directory where burst files should be written to.")

	(OPTION_BKM_DIR, po::value<std::string>()->required(),
			"Path to the directory where the bkm files with the path to the original burst file and some statistics should be written after finishing each burst.")

	(OPTION_THREAD_NUM, po::value<int>()->default_value(0), "Number of worker threads. If it is 0 the number of CPU cores will be used")

	(
	OPTION_DIM_UPDATE_TIME, po::value<int>()->required(), "Milliseconds to sleep between two monitor updates.")

	(OPTION_MERGER_ID, po::value<int>()->required(), "Identifier of this merger. This will be the first number of the created files.")

	(OPTION_RUN_NUMBER, po::value<int>()->default_value(0),
			"Current run number (will be updated via dim). Set to 0 if you want to wait for the dim info before starting")

	(OPTION_TIMEOUT, po::value<int>()->required(), "If we didn't receive <burstTimeout> seconds any event from one burst the file will be written.")

	(OPTION_EOB_COLLECTION_TIMEOUT, po::value<int>()->default_value(500), "Wait this many ms after an EOB before reading the EOB services")

	(OPTION_APPEND_DIM_EOB, po::value<int>()->default_value(1), "Set to 0 if no dim EOB data should be appended to the EOB events")

	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count(OPTION_HELP)) {
		std::cout << desc << "\n";
		exit(EXIT_SUCCESS);
	}

	std::cout << "=======Reading config file " << vm[OPTION_CONFIG_FILE ].as<std::string>() << std::endl;
	po::store(po::parse_config_file<char>(vm[OPTION_CONFIG_FILE ].as<std::string>().data(), desc), vm);
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm); // Check the configuration

	std::cout << "=======Running with following configuration:" << std::endl;
	PrintVM(vm);

	VERBOSE = vm.count(OPTION_VERBOSE) > 0;

	/*
	 * Listening port
	 */
	LISTEN_PORT = vm[OPTION_LISTEN_PORT ].as<int>();

	/*
	 * Storage
	 */
	STORAGE_DIR = vm[OPTION_STORAGE_DIR ].as<std::string>();
	if (!boost::filesystem::exists(STORAGE_DIR)) {
		throw BadOption(OPTION_STORAGE_DIR, "Directory does not exist!");
	}

	BKM_DIR = vm[OPTION_BKM_DIR ].as<std::string>();
	if (!boost::filesystem::exists(BKM_DIR)) {
		throw BadOption(OPTION_BKM_DIR, "Directory does not exist!");
	}

	MONITOR_UPDATE_TIME = vm[OPTION_DIM_UPDATE_TIME ].as<int>();

	MERGER_ID = vm[OPTION_MERGER_ID ].as<int>();

	RUN_NUMBER = vm[OPTION_RUN_NUMBER ].as<int>();

	TIMEOUT = vm[OPTION_TIMEOUT ].as<int>();

	EOB_COLLECTION_TIMEOUT = vm[OPTION_EOB_COLLECTION_TIMEOUT ].as<int>();

	APPEND_DIM_EOB = vm[OPTION_APPEND_DIM_EOB ].as<int>();
}
} /* namespace merger */
} /* namespace na62 */
