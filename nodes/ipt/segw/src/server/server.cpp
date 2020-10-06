/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */


#include "server.h"
#include "session.h"
#include "../cache.h"

namespace node
{
	namespace sml
	{

		server::server(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& cfg
			, storage& db
			, std::string account
			, std::string pwd
			, bool accept_all
			, boost::asio::ip::tcp::endpoint ep)
		: mux_(mux)
			, logger_(logger)
			, cache_(cfg)
			, storage_(db)
			, account_(account)
			, pwd_(pwd)
			, accept_all_(accept_all)
			, acceptor_(mux.get_io_service(), ep)
			, session_counter_{ 0 }
		{
			CYNG_LOG_INFO(logger_, "configured SML local endpoint " << ep);
		}

		void server::run()
		{
			CYNG_LOG_INFO(logger_, "start SML server: " << acceptor_.local_endpoint());
			do_accept();

			//
			//	update status word: service interface available
			//
			cache_.set_status_word(sml::STATUS_BIT_SERVICE_IF_AVAILABLE, true);
		}

		void server::do_accept()
		{
			acceptor_.async_accept(
				[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {

					if (!ec)
					{
						CYNG_LOG_TRACE(logger_, "start SML session at " << socket.remote_endpoint());

						//
						//	There is no connection manager or list of open connections. 
						//	Connections are managed by there own and are controlled
						//	by a maintenance task.
						//
						auto sp = std::shared_ptr<session>(new session(std::move(socket)
							, mux_
							, logger_
							, cache_
							, storage_
							, account_
							, pwd_
							, accept_all_
						), [this](session* s) {

							//
							//	update session counter
							//
							--session_counter_;
							CYNG_LOG_TRACE(logger_, "SML session count " << session_counter_);

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

						do_accept();
					}

				});
		}

		void server::close()
		{
			// The server is stopped by cancelling all outstanding asynchronous
			// operations. Once all operations have finished the io_context::run()
			// call will exit.
			acceptor_.cancel();
			acceptor_.close();
		}

	}
}



