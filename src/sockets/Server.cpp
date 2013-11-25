#include "Server.hpp"

#include <zmq.h>
#include <zmq.hpp>
#include <cstring>
#include <iostream>

#include "../merger/Merger.hpp"
#include "../options/Options.h"
#include "Event.hpp"

namespace na62 {
namespace merger {

Server::Server(Merger& merger, zmq::context_t *context, uint threadNum) :
		thread_(&Server::thread, this), merger_(merger), threadNum_(threadNum), context_(context) {

}

Server::~Server() {
	/*
	 * Do NOT delete dataBuffer_ as this is still used in the merger. headerBuffer_ is
	 * recreated after every packet so it must be deleted when the Server closes!
	 */
}

void Server::thread() {
	zmq::socket_t socket(*context_, ZMQ_PULL);

	std::stringstream bindURI;
	bindURI << "tcp://*:" << Options::FIRST_LISTEN_PORT + threadNum_;

	std::cout << "Opening ZMQ socket " << bindURI.str() << std::endl;
	socket.bind(bindURI.str().c_str());

	while (true) {
		//  Wait for next request from client
		zmq::message_t eventMessage;
		socket.recv(&eventMessage);
		EVENT* event = reinterpret_cast<EVENT*>(new char[eventMessage.size()]);
		memcpy(event, eventMessage.data(), eventMessage.size());

		if (eventMessage.size() == event->length * 4) {
			merger_.addPacket(event);
		} else {
			std::cerr << "Received " << eventMessage.size() << " Bytes with an event of length " << (event->length * 4) << std::endl;
		}
	}
}

} // namespace merger
} // namespace na62
