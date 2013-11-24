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
		thread_(&Server::thread, this), merger_(merger), context_(context), threadNum_(threadNum) {

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
	bindURI << "tcp://*:" << Options::LISTEN_PORT + threadNum_;

	std::cout << "Opening ZMQ socket " << bindURI.str() << std::endl;
	socket.bind(bindURI.str().c_str());

	while (true) {
		//  Wait for next request from client
		zmq::message_t request;
		socket.recv(&request);
		std::cout << "Received request: [" << (char*) request.data() << "]" << std::endl;
		char* event = new char[request.size()];

		memcpy(event, request.data(), request.size());
		merger_.addPacket(reinterpret_cast<EVENT*>(event));
	}
}

} // namespace merger
} // namespace na62
