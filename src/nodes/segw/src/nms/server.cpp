/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <nms/server.h>
#include <nms/session.h>

#include <cyng/task/controller.h>
#include <cyng/log/record.h>

namespace smf {
	namespace nms {

		server::server(cyng::controller& ctl, cfg& c, cyng::logger logger)
			: cfg_(c)
			, logger_(logger)
			, acceptor_(ctl.get_ctx())
			, session_counter_{ 0 }
		{}

		void server::start(boost::asio::ip::tcp::endpoint ep) {
			bool already_openend = false;
			bool already_set_options= false;
			bool already_bind =false;
			bool already_listen = false;
			int sleep_interval_seconds = 2;
			int cnt = 0;
			const int MAX_COUNT = 10;

			boost::system::error_code ec;

			while ( (cnt < MAX_COUNT ) && ( !already_listen ) )  
			{
				CYNG_LOG_INFO(logger_, "[NMS] server: loop no.  " << cnt <<" of " << MAX_COUNT-1);
				cnt++;

				if ( !already_openend )
				{
					acceptor_.open(ep.protocol(), ec);
					if (ec)
					{
						CYNG_LOG_INFO(logger_, "[NMS] server: open failed ")
						CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds")
						sleep(sleep_interval_seconds);
						sleep_interval_seconds *= 2;
						continue;
					}else {
						already_openend = true;
						CYNG_LOG_INFO(logger_, "[NMS] server: open sucessful ")
					}

				}else{
					CYNG_LOG_INFO(logger_, "[NMS] server: already opened ")
				}

				if ( !already_set_options )
				{
						acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
					if (ec){
						CYNG_LOG_INFO(logger_, "[NMS] server: set option failed ")
						CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds");
						sleep(sleep_interval_seconds);
						sleep_interval_seconds *= 2;
						continue;
					}
					else{
						already_set_options= true;
						CYNG_LOG_INFO(logger_, "[NMS] server: set options sucessful ")

					}
				}
				if ( !already_bind )
				{
					acceptor_.bind(ep, ec);
					if (ec){
							CYNG_LOG_INFO(logger_, "[NMS] server: bind failed ")
							CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds");
							sleep(sleep_interval_seconds);
							sleep_interval_seconds *= 2;
							continue;
					}
					else{
						already_bind = true;
						CYNG_LOG_INFO(logger_, "[NMS] server: bind sucessful ")

					}
				}

				if ( !already_listen )
				{
					acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
					if (ec){
							CYNG_LOG_INFO(logger_, "[NMS] server: listen failed ")
							CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds");
							sleep(sleep_interval_seconds);
							sleep_interval_seconds *= 2;
							continue;
					}
					else{
						already_listen= true;
						CYNG_LOG_INFO(logger_, "[NMS] server: listen sucessful ")

					}
				}
			}

			if (already_listen){
				do_accept();
			}
			else {
				CYNG_LOG_WARNING(logger_, "[NMS] server cannot start listening at " 
					<< ep << ": " << ec.message());
			}
		}

		void server::do_accept() {
			acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
				if (!ec) {
					CYNG_LOG_INFO(logger_, "[NMS] new session " << socket.remote_endpoint());

					auto sp = std::shared_ptr<session>(new session(
						std::move(socket), 
						cfg_, 
						logger_,
						std::bind(&server::rebind, this, std::placeholders::_1)
					), [this](session* s) {

						//
						//	update session counter
						//
						--session_counter_;
						CYNG_LOG_TRACE(logger_, "[NMS] session(s) running: " << session_counter_);

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

					//
					//	continue listening
					//
					do_accept();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[NMS] server stopped: " << ec.message());
				}
			});
		}

		void server::stop() {
			acceptor_.cancel();
			acceptor_.close();
		}

		void server::rebind(boost::asio::ip::tcp::endpoint ep) {

			acceptor_ = boost::asio::ip::tcp::acceptor(acceptor_.get_executor(), ep);

		}

	}
}