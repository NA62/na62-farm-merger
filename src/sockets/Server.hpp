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

#include "../utils/AExecutable.h"

#define SERVER_ADDR "inproc://worker"

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

class Server: public AExecutable {
public:
	explicit Server(Merger& merger, zmq::context_t *context);

	~Server();

	void thread();
private:
	Merger& merger_;
	zmq::context_t *context_;

};

} // namespace merger
} // namespace na62

#endif // na62_merger_Server_HPP
