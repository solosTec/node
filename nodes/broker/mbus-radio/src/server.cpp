/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "server.h"
#include "session.h"
#include <smf/cluster/generator.h>

#include <cyng/vm/controller.h>

namespace node
{
	server::server(cyng::io_service_t& ios
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, boost::asio::ip::tcp::endpoint ep)
	: acceptor_(ios, ep)
		, logger_(logger)
		, vm_(vm)
		, session_counter_{ 0 }
	{
		CYNG_LOG_INFO(logger_, "configured local endpoint " << ep);
	}

	void server::run()
	{
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
					CYNG_LOG_TRACE(logger_, "start session at " << socket.remote_endpoint());

					auto sp = std::shared_ptr<session>(new session(std::move(socket), logger_, vm_), [this](session* s) {

						//
						//	update session counter
						//
						--session_counter_;
						CYNG_LOG_TRACE(logger_, "session count " << session_counter_);
						if (!vm_.is_halted()) {
							vm_.async_run(bus_update_client_count(session_counter_));
						}

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
						vm_.async_run(bus_update_client_count(session_counter_));

					}
				}

				do_accept();
			});
	}

}
