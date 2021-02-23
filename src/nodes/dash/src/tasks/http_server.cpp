#include <tasks/http_server.h>

#include <smf/http/session.h>
#include <smf/http/ws.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	http_server::http_server(cyng::channel_weak wp
		, boost::asio::io_context& ioc
		, boost::uuids::uuid tag
		, cyng::logger logger
		, std::string const& document_root
		, std::uint64_t max_upload_size
		, std::string const& nickname
		, std::chrono::seconds timeout)
		: sigs_{
			std::bind(&http_server::stop, this, std::placeholders::_1),
			std::bind(&http_server::listen, this, std::placeholders::_1),
	}
	, channel_(wp)
		, tag_(tag)
		, logger_(logger)
		, document_root_(document_root)
		, max_upload_size_(max_upload_size)
		, nickname_(nickname)
		, timeout_(timeout)
		, server_(ioc, logger, std::bind(&http_server::accept, this, std::placeholders::_1))
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("listen", 1);
		}

		CYNG_LOG_INFO(logger_, "http server " << tag << " started");

	}

	http_server::~http_server()
	{
#ifdef _DEBUG_DASH
		std::cout << "http_server(~)" << std::endl;
#endif
	}


	void http_server::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop http server task(" << tag_ << ")");
		server_.stop();
	}

	void http_server::listen(boost::asio::ip::tcp::endpoint ep) {

		CYNG_LOG_INFO(logger_, "start listening at(" << ep << ")");
		server_.start(ep);

	}

	void http_server::accept(boost::asio::ip::tcp::socket&& s) {

		CYNG_LOG_INFO(logger_, "accept (" << s.remote_endpoint() << ")");

		std::make_shared<http::session>(
			std::move(s),
			logger_,
			document_root_,
			max_upload_size_,
			nickname_,
			timeout_,
			std::bind(&http_server::upgrade, this, std::placeholders::_1, std::placeholders::_2))->run();
	}

	void http_server::upgrade(boost::asio::ip::tcp::socket s
		, boost::beast::http::request<boost::beast::http::string_body> req) {

		CYNG_LOG_TRACE(logger_, "ws (" << s.remote_endpoint() << ")");
		std::make_shared<http::ws>(std::move(s), logger_)->do_accept(std::move(req));
	}
}


