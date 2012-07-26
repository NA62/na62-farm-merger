//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at na62://www.boost.org/LICENSE_1_0.txt)
//

#ifndef na62_merger_CONNECTION_HPP
#define na62_merger_CONNECTION_HPP

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "../merger/Merger.hpp"

#include "request.hpp"

namespace na62 {
namespace merger {

class Receiver;
/// Represents a single connection from a client.
class Connection: public boost::enable_shared_from_this<Connection>, private boost::noncopyable {
public:
	/// Construct a connection with the given io_service.
	explicit Connection(boost::asio::io_service& io_service, Merger& merger, Receiver* receiver);

	~Connection();

	/// Get the socket associated with the connection.
	boost::asio::ip::tcp::socket& socket();

	void async_readHeader();

private:
	/// Handle completion of a read operation.
	void handle_readHeader(const boost::system::error_code& e, std::size_t bytes_transferred);
	void handle_readData(const boost::system::error_code& e, std::size_t bytes_transferred);

	/// Handle completion of a write operation.
	void handle_write(const boost::system::error_code& e);

	/// Strand to ensure the connection's handlers are not called concurrently.
	boost::asio::io_service::strand strand_;

	/// Socket for the connection.
	boost::asio::ip::tcp::socket socket_;

	/// The handler used to process the incoming request.
	Merger& merger_;

	Receiver* receiver_;

	/// Buffer for incoming data.
//	boost::array<char, sizeof(EVENT_HDR)> headerBuffer_;
	char* headerBuffer_;
	char* dataBuffer_;

	int lastBurstID_;
};

typedef boost::shared_ptr<Connection> connection_ptr;

} // namespace merger
} // namespace na62

#endif // na62_merger_CONNECTION_HPP
