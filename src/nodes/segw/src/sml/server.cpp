/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <sml/server.h>

#include <cyng/task/controller.h>
#include <cyng/log/record.h>

namespace smf {
	namespace sml {

		server::server(cyng::controller& ctl, cfg& c, cyng::logger logger)
			: cfg_(c)
			, logger_(logger)
			, acceptor_(ctl.get_ctx())
		{
		}

		void server::start(boost::asio::ip::tcp::endpoint ep) {
			boost::system::error_code ec;
			acceptor_.open(ep.protocol());
			if (!ec)	acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
			if (!ec)	acceptor_.bind(ep, ec);
			if (!ec)	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
			if (!ec) {
				acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
				do_accept();
			}
			else {
				CYNG_LOG_WARNING(logger_, "[SML] server cannot start listening at "
					<< ep << ": " << ec.message());
			}
		}

		void server::do_accept() {
			acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
				if (!ec) {
					CYNG_LOG_INFO(logger_, "[SML] new session " << socket.remote_endpoint());

					//
					//	continue listening
					//
					do_accept();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[SML] server stopped: " << ec.message());
				}
			});
		}
		void server::stop() {
			acceptor_.cancel();
			acceptor_.close();
		}

	}
}