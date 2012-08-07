#include <boost/bind.hpp>

#include "Connection.hpp"
#include "Receiver.hpp"

#include "../utils/Utils.h"

namespace na62 {
namespace merger {

Connection::Connection(boost::asio::io_service& io_service, Merger& merger, Receiver* receiver) :
		io_service_(io_service), strand_(io_service), socket_(io_service), merger_(merger), receiver_(receiver), headerBuffer_(NULL), dataBuffer_(
				NULL), lastBurstID_(0) {
}

Connection::~Connection() {
	delete[] headerBuffer_;
	/*
	 * Do NOT delete dataBuffer_ as this is still used in the merger. headerBuffer_ is
	 * recreated after every packet so it must be deleted when the connection closes!
	 */
}

boost::asio::ip::tcp::socket&
Connection::socket() {
	return socket_;
}

void Connection::run() {
	boost::system::error_code error;

	while (true) {
		headerBuffer_ = new char[sizeof(EVENT_HDR)];

		boost::asio::read(socket_, boost::asio::buffer(headerBuffer_, sizeof(struct EVENT_HDR)),
				boost::asio::transfer_exactly(sizeof(struct EVENT_HDR)), error);

		if (error == boost::asio::error::eof) {
			merger_.handle_burstFinished(socket().remote_endpoint().address().to_string(), lastBurstID_);
			break;
		}

		EVENT_HDR* hdr = (EVENT_HDR*) headerBuffer_;
		lastBurstID_ = hdr->burstID;

		//		std::cerr << "Header received: " << (int) bytes_transferred << "B " << (int) hdr->eventNum << " Events with " << (int) hdr->numberOfDetectors
		//				<< " detectors " << std::endl;

		if (hdr->length == 0 && hdr->timestamp == 0) { // Special header: new burst!
			if (hdr->format == 0) {
				merger_.handle_newBurst(socket().remote_endpoint().address().to_string(), hdr->burstID);
			} else {
				merger_.handle_burstFinished(socket().remote_endpoint().address().to_string(), hdr->burstID);
			}
			continue;
		}

		std::size_t expectedDataBytes = hdr->length * 4 - sizeof(struct EVENT_HDR);

		dataBuffer_ = new char[expectedDataBytes];

		boost::asio::read(socket_, boost::asio::buffer(dataBuffer_, expectedDataBytes), boost::asio::transfer_exactly(expectedDataBytes));

		if (error == boost::asio::error::eof) {
			merger_.handle_burstFinished(socket().remote_endpoint().address().to_string(), lastBurstID_);
			break;
		}

		struct EVENT e = { (EVENT_HDR*) headerBuffer_, dataBuffer_ };
		merger_.addPacket(e);
	}
	std::cout << "Connection closed with " << socket_.remote_endpoint().address().to_string() << std::endl;
	socket_.close();
}

} // namespace merger
} // namespace na62
