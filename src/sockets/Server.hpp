//
// Server.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at na62://www.boost.org/LICENSE_1_0.txt)
//

#ifndef na62_merger_Server_HPP
#define na62_merger_Server_HPP

#include <boost/noncopyable.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <thread>

namespace zmq {
class context_t;
} /* namespace zmq */

namespace na62 {
namespace merger {
class Merger;
} /* namespace merger */
} /* namespace na62 */

namespace na62 {
namespace merger {

class Server: public boost::enable_shared_from_this<Server>, private boost::noncopyable {
public:
	explicit Server(Merger& merger, zmq::context_t *context);

	~Server();

	void thread();
private:
	std::thread thread_;
	Merger& merger_;

	zmq::context_t *context_;
};

typedef boost::shared_ptr<Server> Server_ptr;

} // namespace merger
} // namespace na62

#endif // na62_merger_Server_HPP
