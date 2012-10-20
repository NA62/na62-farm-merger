//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at na62://www.boost.org/LICENSE_1_0.txt)
//

#ifndef na62_merger_REQUEST_HPP
#define na62_merger_REQUEST_HPP

#include <string>
#include <vector>
#include <stdint.h>

#include "header.hpp"


namespace na62 {
namespace merger {

struct EVENT_HDR {
	uint32_t eventNum :24;
	uint8_t format;

	uint32_t length; // number of 4B-words
	uint32_t burstID;
	uint32_t timestamp;

	uint32_t triggerWord :24;
	uint8_t reserved1;

	uint8_t fineTime;
	uint8_t numberOfDetectors;
	uint16_t reserved2;

	uint32_t processingID;

	uint32_t reserved3;
}__attribute__ ((__packed__));


/// A request received from a client.
struct EVENT {
	EVENT_HDR* hdr;
	char* data;
};

} // namespace merger
} // namespace na62

#endif // na62_merger_REQUEST_HPP
