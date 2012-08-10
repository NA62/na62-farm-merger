#include "Receiver.hpp"
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <vector>

namespace na62 {
namespace merger {

Receiver::Receiver(const std::string& address, const std::string& port, std::size_t thread_pool_size) :
		thread_pool_size_(thread_pool_size), signals_(io_service_), acceptor_(io_service_), connection_(), merger_() {
	// Register to handle the signals that indicate when the server should exit.
	// It is safe to register for the same signal multiple times in a program,
	// provided all registration for the specified signal is made through Asio.
	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
	signals_.async_wait(boost::bind(&Receiver::handle_stop, this));

	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	start_accept();
}

Receiver::~Receiver() {
	acceptor_.close();
}

void Receiver::run() {
	// Create a pool of threads to run all of the io_services.
	boost::thread_group threads;
	for (std::size_t i = 0; i < thread_pool_size_; ++i) {
		threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
	}

	threads.join_all();
}

void Receiver::start_accept() {
	connection_.reset(new Connection(io_service_, merger_, this));
	acceptor_.async_accept(connection_->socket(), boost::bind(&Receiver::handle_accept, this, boost::asio::placeholders::error));
}

void Receiver::handle_accept(const boost::system::error_code& e) {
	if (!e) {
		std::cout << "New client connected: " << connection_->socket().remote_endpoint().address().to_string() << std::endl;
		boost::thread(boost::bind(&Connection::run, connection_));
	} else {
		std::cerr << "Error: " << e.message() << std::endl;
	}
	start_accept();
}

void Receiver::handle_stop() {
	std::cout << "Stopping Receiver" << std::endl;
	io_service_.stop();
}

} // namespace merger
} // namespace na62
