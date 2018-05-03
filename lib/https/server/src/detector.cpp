/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "detect_ssl.hpp"
#include <smf/https/srv/detector.h>
#include <smf/https/srv/session.h>
#include <cyng/vm/generator.h>
#include <cyng/object_cast.hpp>

namespace node
{
	namespace https
	{
		detector::detector(cyng::logging::log_ptr logger
			, server_callback_t cb
			, boost::asio::ip::tcp::socket socket
			, boost::asio::ssl::context& ctx
			, std::string const& doc_root
			, std::vector<std::string> const& sub_protocols)
		: logger_(logger)
			, cb_(cb)
			, socket_(std::move(socket))
			, ctx_(ctx)
			, strand_(socket_.get_executor())
			, doc_root_(doc_root)
			, sub_protocols_(sub_protocols)
			, buffer_()
			, rgn_()
		{}

		void detector::run()
		{
			async_detect_ssl(
				socket_,
				buffer_,
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&detector::on_detect,
						shared_from_this(),
						std::placeholders::_1,
						std::placeholders::_2)));

		}

		void detector::on_detect(boost::system::error_code ec, boost::tribool result)
		{
			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "detect: " << ec.message());
				return;
			}

			if (result)
			{
				// Launch SSL session
				CYNG_LOG_DEBUG(logger_, "launch HTTP SSL session");
				auto obj = cyng::make_object<ssl_session>(logger_
					, std::bind(cb_, rgn_(), std::placeholders::_1)
					, std::move(socket_)
					, ctx_
					, std::move(buffer_)
					, doc_root_
					, sub_protocols_);
				const_cast<ssl_session*>(cyng::object_cast<ssl_session>(obj))->run(obj);
				//std::make_shared<ssl_session>(logger_
				//	, std::bind(cb_, rgn_(), std::placeholders::_1)
				//	, std::move(socket_)
				//	, ctx_
				//	, std::move(buffer_)
				//	, doc_root_
				//	, sub_protocols_)->run();
				return;
			}

			// Launch plain session
			CYNG_LOG_DEBUG(logger_, "launch plain HTTP session");
			//cb_(cyng::generate_invoke("https.launch.session.plain", socket_.remote_endpoint()));
			auto obj = cyng::make_object<plain_session>(logger_
				, std::bind(cb_, rgn_(), std::placeholders::_1)
				, std::move(socket_)
				, std::move(buffer_)
				, doc_root_
				, sub_protocols_);
			const_cast<plain_session*>(cyng::object_cast<plain_session>(obj))->run(obj);
			//std::make_shared<plain_session>(logger_
			//	, std::bind(cb_, rgn_(), std::placeholders::_1)
			//	, std::move(socket_)
			//	, std::move(buffer_)
			//	, doc_root_
			//	, sub_protocols_)->run();

		}

	}
}

