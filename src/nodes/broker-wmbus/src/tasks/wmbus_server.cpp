#include <tasks/wmbus_server.h>


#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	wmbus_server::wmbus_server(cyng::channel_weak wp
		, boost::asio::io_context& ioc
		, boost::uuids::uuid tag
		, cyng::logger logger)
		: sigs_{
			std::bind(&wmbus_server::stop, this, std::placeholders::_1),
			std::bind(&wmbus_server::listen, this, std::placeholders::_1),
	}
	, channel_(wp)
		, tag_(tag)
		, logger_(logger)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("listen", 1);
		}

		CYNG_LOG_INFO(logger_, "wmbus server " << tag << " started");

	}

	wmbus_server::~wmbus_server()
	{
#ifdef _DEBUG_DASH
		std::cout << "wmbus(~)" << std::endl;
#endif
	}


	void wmbus_server::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop wmbus server task(" << tag_ << ")");
	}

	void wmbus_server::listen(boost::asio::ip::tcp::endpoint ep) {

		CYNG_LOG_INFO(logger_, "start listening at(" << ep << ")");
		//server_.start(ep);

	}

	void wmbus_server::accept(boost::asio::ip::tcp::socket&& s) {

		CYNG_LOG_INFO(logger_, "accept (" << s.remote_endpoint() << ")");

		//std::make_shared<http::session>(
		//	std::move(s),
		//	logger_,
		//	document_root_,
		//	max_upload_size_,
		//	nickname_,
		//	timeout_,
		//	std::bind(&http_server::upgrade, this, std::placeholders::_1, std::placeholders::_2))->run();
	}

}


