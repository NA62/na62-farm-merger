#ifndef na62_merger_SERVER_HPP
#define na62_merger_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Connection.hpp"
#include "../merger/Merger.hpp"

namespace na62 {
namespace merger {

// The top-level class of the merger
class Receiver: private boost::noncopyable {
public:
	explicit Receiver(Merger& merger, const std::string& address, const std::string& port, std::size_t thread_pool_size);
	virtual ~Receiver();
	// Start the server's io_service with a pool of threads.
	void run();

private:
	// Initiate an asynchronous accept operation.
	void start_accept();

	// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& e);

	// Handle a request to stop the server.
	void handle_stop();

	// The number of threads that will call io_service::run().
	std::size_t thread_pool_size_;

	// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	// The signal_set is used to register for process termination notifications.
	boost::asio::signal_set signals_;

	// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	// The next connection to be accepted.
	connection_ptr connection_;

	// The handler for all incoming requests.
	Merger& merger_;
};

} // namespace merger
} // namespace na62

#endif // na62_merger_SERVER_HPP
