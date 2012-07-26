/*
 * Utils.h
 *
 *  Created on: Apr 23, 2011
 *      Author: kunzejo
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <boost/lexical_cast.hpp>
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <string.h>

class Utils {
public:
	Utils();
	static std::string FormatSize(long int size);
	static double Average(std::vector<double> data);
	static double StandardDevation(std::vector<double> data);
	static void PrintHex(const u_char* data, const size_t dataLength);
	static uint ToUInt(std::string str)throw (boost::bad_lexical_cast );
};

#endif /* UTILS_H_ */
