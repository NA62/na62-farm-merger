/*
 * MonitorDimServer.h
 *
 *  Created on: Jun 20, 2012
 *      Author: kunzejo
 */

#ifndef MONITORDIMSERVER_H_
#define MONITORDIMSERVER_H_

#include <dim/dis.hxx>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "merger/Merger.hpp"

namespace na62 {
namespace merger {
typedef boost::shared_ptr<DimService> DimService_ptr;

class MonitorDimServer: public boost::enable_shared_from_this<MonitorDimServer>, DimServer, private boost::noncopyable {
private:
	Merger& merger_;
	const std::string hostName_;

	DimService burstProgressService_;

	boost::asio::io_service updateService_;
	boost::asio::deadline_timer timer_;
public:
	MonitorDimServer(Merger& merger, std::string hostName);

	virtual ~MonitorDimServer();

	void updateStatistics();
};

typedef boost::shared_ptr<MonitorDimServer> MonitorDimServer_ptr;

} /* namespace merger */
}/* namespace na62 */
#endif /* MONITORDIMSERVER_H_ */
