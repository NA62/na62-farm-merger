/*
 * Options.cpp
 *
 *  Created on: Nov 9, 2011
 *      Author: kunzejo
 */

#include "Options.h"

namespace na62 {

namespace po = boost::program_options;

/*
* Configurable Variables
*/
std::string Options::LISTEN_IP;
std::string Options::LISTEN_PORT;


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
	desc.add_options()
	(OPTION_HELP, "Produce help message")
	(OPTION_CONFIG_FILE, po::value<std::string>()->default_value("/etc/merger.conf"),"Config file for these options")
	(OPTION_LISTEN_IP, po::value<std::string>()->required(), "IP of the device the merger should listen to")
	(OPTION_LISTEN_PORT, po::value<std::string>()->required(), "IP tcp-port the merger should listen to");

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


	/*
	* Listening Devic/IP
	*/
	LISTEN_IP = vm[OPTION_LISTEN_IP ].as<std::string>();
	LISTEN_PORT = vm[OPTION_LISTEN_PORT ].as<std::string>();;
}

} /* namespace na62 */
