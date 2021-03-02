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
			boost::system::error_code ec;
			acceptor_.open(ep.protocol(), ec);
			if (!ec)	acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
			if (!ec)	acceptor_.bind(ep, ec);
			if (!ec)	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
			if (!ec) {
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