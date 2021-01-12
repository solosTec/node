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
#include <tasks/rebind.h>

#include <cyng/vm/controller.h>
#include <cyng/vm/context.h>
#include <cyng/io/serializer.h>
#include <cyng/async/task/task_builder.hpp>

namespace node
{
	namespace nms
	{
		server::server(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& cfg
			, std::string account
			, std::string pwd
			, boost::asio::ip::tcp::endpoint ep)
		: mux_(mux)
			, logger_(logger)
			, cache_(cfg)
			, account_(account)
			, pwd_(pwd)
			, acceptor_(mux.get_io_service(), ep)
			, session_counter_{ 0 }
			//
			//	start rebind task
			//
			, tsk_rebind_{ cyng::async::start_task_detached<rebind>(mux
				, logger_
				, acceptor_) }
		{
			acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
		}

		void server::run()
		{
			CYNG_LOG_INFO(logger_, "NMS server listens at: " 
				<< acceptor_.local_endpoint()
			<< " - rebind task #"
			<< tsk_rebind_);
			do_accept();
		}

		void server::close()
		{
			mux_.stop(tsk_rebind_);
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

						auto sp = std::shared_ptr<session>(new session(mux_, std::move(socket),logger_, cache_, account_, pwd_, tsk_rebind_), [this](session* s) {

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
