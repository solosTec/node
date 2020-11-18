/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <nms/server.h>
#include <nms/session.h>
#include <cache.h>
#include <smf/cluster/generator.h>

#include <cyng/vm/controller.h>
#include <cyng/vm/context.h>
#include <cyng/io/serializer.h>

namespace node
{
	namespace nms
	{
		server::server(cyng::io_service_t& ios
			, cyng::logging::log_ptr logger
			, cache& cfg
			, std::string account
			, std::string pwd
			, boost::asio::ip::tcp::endpoint ep)
		: logger_(logger)
			, cache_(cfg)
			, account_(account)
			, pwd_(pwd)
			, acceptor_(ios, ep)
			, session_counter_{ 0 }
		{}

		void server::run()
		{
			CYNG_LOG_INFO(logger_, "NMS server listens at: " << acceptor_.local_endpoint());
			do_accept();
		}

		void server::close()
		{
			acceptor_.cancel();
			acceptor_.close();
		}

		void server::do_accept()
		{
			acceptor_.async_accept(
				[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
				{


					if (!ec)
					{
						CYNG_LOG_TRACE(logger_, "start NMS session at " << socket.remote_endpoint());

						auto sp = std::shared_ptr<session>(new session(std::move(socket), logger_, cache_, account_, pwd_), [this](session* s) {

							//
							//	update session counter
							//
							--session_counter_;
							CYNG_LOG_TRACE(logger_, "NMS session count " << session_counter_);

							//
							//	remove session
							//
							delete s;
							});

						if (sp) {

							//
							//	start session
							//
							sp->start();

							//
							//	update session counter
							//
							++session_counter_;

						}
					}

					do_accept();
				});
		}
	}
}
