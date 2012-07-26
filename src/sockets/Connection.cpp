#include <boost/bind.hpp>

#include "Connection.hpp"
#include "Receiver.hpp"

#include "../utils/Utils.h"

namespace na62 {
namespace merger {

Connection::Connection(boost::asio::io_service& io_service, Merger& merger, Receiver* receiver) :
		strand_(io_service), socket_(io_service), merger_(merger), receiver_(receiver), headerBuffer_(NULL), dataBuffer_(NULL), lastBurstID_(0) {
}

Connection::~Connection() {
	std::cout << "Connection closed with " << socket_.remote_endpoint().address().to_string() << std::endl;
	delete[] headerBuffer_;
	socket_.close();
	/*
	 * Do NOT delete dataBuffer_ as this is still used in the merger. headerBuffer_ is
	 * recreated after every packet so it must be deleted when the connection closes!
	 */
}

boost::asio::ip::tcp::socket&
Connection::socket() {
	return socket_;
}

void Connection::async_readHeader() {
	headerBuffer_ = new char[sizeof(EVENT_HDR)];

	boost::asio::async_read(socket_, boost::asio::buffer(headerBuffer_, sizeof(struct EVENT_HDR)), boost::asio::transfer_all(),
			strand_.wrap(
					boost::bind(&Connection::handle_readHeader, shared_from_this(), boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred)));
}

void Connection::handle_readHeader(const boost::system::error_code& e, std::size_t bytes_transferred) {
	if (!e) {
		std::cerr << "Header received: "<<bytes_transferred << std::endl;
		EVENT_HDR* hdr = (EVENT_HDR*) headerBuffer_;
		lastBurstID_ = hdr->burstID;

		if (hdr->length == 0 && hdr->timestamp == 0) { // Special header: new burst!
			if (hdr->format == 0) {
				merger_.handle_newBurst(socket().remote_endpoint().address().to_string(), hdr->burstID);
				async_readHeader();
			} else {
				merger_.handle_burstFinished(socket().remote_endpoint().address().to_string(), hdr->burstID);
				async_readHeader();
			}
			return;
		}

		std::size_t expectedDataBytes = hdr->length * 4 - sizeof(struct EVENT_HDR);

//		std::cerr << (int) hdr->eventNum << " : " << (int) hdr->format << " : " << (int) hdr->length << " : " << (int) hdr->burstID << " : "
//				<< (int) hdr->timestamp << " : " << (int) hdr->triggerWord << " : " << (int) hdr->fineTime << std::endl;

		dataBuffer_ = new char[expectedDataBytes];
		boost::asio::async_read(socket_, boost::asio::buffer(dataBuffer_, expectedDataBytes), boost::asio::transfer_exactly(expectedDataBytes),
				strand_.wrap(
						boost::bind(&Connection::handle_readData, shared_from_this(), boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred)));
	} else {
		if (e == boost::asio::error::eof) {
			std::cout << "Connection closed with " << socket_.remote_endpoint().address().to_string() << std::endl;
			merger_.handle_burstFinished(socket().remote_endpoint().address().to_string(), lastBurstID_);
		} else {
			std::cerr << "Error: " << e.message() << std::endl;
		}
	}

	// If an error occurs then no new asynchronous operations are started. This
	// means that all shared_ptr references to the connection object will
	// disappear and the object will be destroyed automatically after this
	// handler returns. The connection class's destructor closes the socket.
}

void Connection::handle_readData(const boost::system::error_code& e, std::size_t bytes_transferred) {
	if (!e) {
		std::cerr << "Data  received: "<<bytes_transferred << std::endl;
		struct EVENT e = { (EVENT_HDR*) headerBuffer_, dataBuffer_ };
		merger_.addPacket(e);
		async_readHeader();
	} else {
		std::cerr << "Error: " << e.message() << std::endl;
	}

	// If an error occurs then no new asynchronous operations are started. This
	// means that all shared_ptr references to the connection object will
	// disappear and the object will be destroyed automatically after this
	// handler returns. The connection class's destructor closes the socket.
}

} // namespace merger
} // namespace na62
