#include "Server.hpp"

//#include <sys/types.h>
#include <zmq.h>
#include <zmq.hpp>
#include <cstring>
#include <iostream>
#include <structs/Event.h>

#include "../merger/Merger.hpp"

namespace na62 {
namespace merger {

Server::Server(Merger& merger, zmq::context_t *context) :
		merger_(merger), context_(context) {

}

Server::~Server() {
	/*
	 * Do NOT delete dataBuffer_ as this is still used in the merger. headerBuffer_ is
	 * recreated after every packet so it must be deleted when the Server closes!
	 */
}

void Server::thread() {
	zmq::socket_t socket(*context_, ZMQ_PULL);

	socket.connect(SERVER_ADDR);

	while (true) {
		//  Wait for next request from client
		zmq::message_t* eventMessage = new zmq::message_t();
		socket.recv(eventMessage);
		EVENT_HDR* event = reinterpret_cast<EVENT_HDR*>(eventMessage->data());

		/*
		 * TODO: do we really have to memcpy here?
		 */
//		memcpy(event, eventMessage.data(), eventMessage.size());

		if (eventMessage->size() == event->length * 4) {
			merger_.addPacket(eventMessage);
		} else {
			std::cerr << "Received " << eventMessage->size() << " Bytes with an event of length " << (event->length * 4) << std::endl;
			delete eventMessage;
		}
	}
}

} // namespace merger
} // namespace na62
