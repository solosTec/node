#include <http_server.h>

#include <smf/http/session.h>
#include <smf/http/ws.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	http_server::http_server(boost::asio::io_context& ioc
		, boost::uuids::uuid tag
		, cyng::logger logger
		, std::string const& document_root
		, std::uint64_t max_upload_size
		, std::string const& nickname
		, std::chrono::seconds timeout
		, blocklist_type&& blocklist)
	: tag_(tag)
		, logger_(logger)
		, document_root_(document_root)
		, max_upload_size_(max_upload_size)
		, nickname_(nickname)
		, timeout_(timeout)
		, blocklist_(blocklist.begin(), blocklist.end())
		, server_(ioc, logger, std::bind(&http_server::accept, this, std::placeholders::_1))
		, parser_([this](cyng::object&& obj) {

			CYNG_LOG_TRACE(logger_, "[HTTP] ws parsed " << obj);
			//send_response(reader_.run(cyng::container_cast<cyng::param_map_t>(std::move(obj)), rebind));

		})
		, uidgen_()
	{
		CYNG_LOG_INFO(logger_, "[HTTP] server " << tag << " started");
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

		//
		//	check blocklist
		//
		auto const pos = blocklist_.find(s.remote_endpoint().address());
		if (pos != blocklist_.end()) {

			CYNG_LOG_WARNING(logger_, "address ["
				<< s.remote_endpoint()
				<< "] is blocked");
			s.close();

			return;
		}

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

		CYNG_LOG_INFO(logger_, "new ws (" << s.remote_endpoint() << ")");
		std::make_shared<http::ws>(std::move(s)
			, logger_
			, std::bind(&http_server::on_msg, this, uidgen_(), std::placeholders::_1))->do_accept(std::move(req));
	}

	void http_server::on_msg(boost::uuids::uuid tag, std::string msg) {

		//
		//	JSON parser
		//
		CYNG_LOG_TRACE(logger_, "ws (" << tag << ") received " << msg);
		parser_.read(msg.begin(), msg.end());
	}

}


