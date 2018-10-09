/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "detect_ssl.hpp"
#include <smf/https/srv/detector.h>
#include <smf/https/srv/session.h>
#include <smf/https/srv/connections.h>

#include <cyng/vm/generator.h>
#include <cyng/object_cast.hpp>

#include <boost/uuid/uuid_io.hpp>

namespace node
{
	namespace https
	{
		detector::detector(cyng::logging::log_ptr logger
			, connections& cm
			, boost::uuids::uuid tag
			, boost::asio::ip::tcp::socket socket
			, boost::asio::ssl::context& ctx
			, std::string const& doc_root)
		: logger_(logger)
			, connection_manager_(cm)
			, tag_(tag)
			, socket_(std::move(socket))
			, ctx_(ctx)
			, strand_(socket_.get_executor())
			, doc_root_(doc_root)
			, buffer_()
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
						shared_from_this(),	//	reference
						std::placeholders::_1,
						std::placeholders::_2)));

		}

		void detector::on_detect(boost::system::error_code ec, boost::tribool result)
		{
			if (ec)	{
				CYNG_LOG_ERROR(logger_, "detect: " << ec.message());
				return;
			}

			if (result)	{

				// Launch SSL session
				CYNG_LOG_DEBUG(logger_, "launch SSL session " << tag_);

				auto obj = cyng::make_object<ssl_session>(logger_
					, tag_
					, std::move(socket_)
					, ctx_
					, std::move(buffer_)
					, doc_root_);

				connection_manager_.add_ssl_session(obj);
				return;
			}

			//
			// Launch plain session
			//
			CYNG_LOG_DEBUG(logger_, "launch plain session" << tag_);

			//cb_(cyng::generate_invoke("https.launch.session.plain", socket_.remote_endpoint()));
			auto obj = cyng::make_object<plain_session>(logger_
				, tag_
				, std::move(socket_)
				, std::move(buffer_)
				, doc_root_);

			connection_manager_.add_plain_session(obj);

		}

	}
}

