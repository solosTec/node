/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#if (BOOST_BEAST_VERSION < 248)
#include "detect_ssl.hpp"
#else
#include <boost/beast/core/detect_ssl.hpp>
#endif

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
			, boost::asio::ip::tcp::socket socket
			, boost::asio::ssl::context& ctx)
		: logger_(logger)
			, connection_manager_(cm)
#if (BOOST_BEAST_VERSION < 248)
			, socket_(std::move(socket))
			, strand_(socket_.get_executor())
#else
			, stream_(std::move(socket))
#endif
			, ctx_(ctx)
			, buffer_()
		{}

		void detector::run()
		{
#if (BOOST_BEAST_VERSION < 248)
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
#elif (BOOST_BEAST_VERSION < 293)
			// Set the timeout.
			stream_.expires_after(std::chrono::seconds(30));

			boost::beast::async_detect_ssl(
				stream_,
				buffer_,
				boost::beast::bind_front_handler(
					&detector::on_detect,
					this->shared_from_this()));
#else
			// We need to be executing within a strand to perform async operations
			  // on the I/O objects in this session. Although not strictly necessary
			  // for single-threaded contexts, this example code is written to be
			  // thread-safe by default.
			boost::asio::dispatch(
				stream_.get_executor(),
				boost::beast::bind_front_handler(
					&detector::on_run,
					this->shared_from_this()));
#endif

		}

#if (BOOST_BEAST_VERSION > 292)
		void detector::on_run() {

			// Set the timeout.
			stream_.expires_after(std::chrono::seconds(30));

			boost::beast::async_detect_ssl(
				stream_,
				buffer_,
				boost::beast::bind_front_handler(
					&detector::on_detect,
					this->shared_from_this()));

		}
#endif

		void detector::on_detect(boost::system::error_code ec, boost::tribool result)
		{
			if (ec)	{
				CYNG_LOG_ERROR(logger_, "detect: " << ec.message());
				return;
			}

			if (result)	{

#if (BOOST_BEAST_VERSION < 248)
				// Launch SSL session
				CYNG_LOG_DEBUG(logger_, "launch SSL session " << socket_.remote_endpoint());

				connection_manager_.add_ssl_session(std::move(socket_)
					, ctx_
					, std::move(buffer_));
#else
				// Launch SSL session
				CYNG_LOG_DEBUG(logger_, "launch SSL session " << stream_.socket().remote_endpoint());

				//BOOST_ASSERT_MSG(false, "ToDo: fix this");
				connection_manager_.add_ssl_session(std::move(stream_)
					, ctx_
					, std::move(buffer_));

#endif

				return;
			}

			//
			// Launch plain session
			//
#if (BOOST_BEAST_VERSION < 248)
			CYNG_LOG_DEBUG(logger_, "launch plain session" << socket_.remote_endpoint());

			connection_manager_.add_plain_session(std::move(socket_)
				, std::move(buffer_));
#else
			CYNG_LOG_DEBUG(logger_, "launch plain session" << stream_.socket().remote_endpoint());

			BOOST_ASSERT_MSG(false, "ToDo: fix this");
			connection_manager_.add_plain_session(std::move(stream_)
				, std::move(buffer_));
#endif

		}

	}
}

