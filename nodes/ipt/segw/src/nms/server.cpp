/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "server.h"
#include "session.h"
#include "../cache.h"
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
			, bool accept_all
			, boost::asio::ip::tcp::endpoint ep)
		: logger_(logger)
			, cache_(cfg)
			, account_(account)
			, pwd_(pwd)
			, accept_all_(accept_all)
			, acceptor_(ios, ep)
			, session_counter_{ 0 }
		{
			CYNG_LOG_INFO(logger_, "configured NSM local endpoint " << ep);
		}

		void server::run()
		{
			CYNG_LOG_INFO(logger_, "start NMS server: " << acceptor_.local_endpoint());
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

						auto sp = std::shared_ptr<session>(new session(std::move(socket), logger_, cache_), [this](session* s) {

							//
							//	update session counter
							//
							--session_counter_;
							CYNG_LOG_TRACE(logger_, "NMS session count " << session_counter_);

							//
							//	remove from VM controller
							//
							//vm_.async_run(cyng::generate_invoke("vm.remove", tag));

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
