#include <wmbus_server.h>
#include <wmbus_session.h>

#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	wmbus_server::wmbus_server(cyng::controller& ctl
		, cyng::logger logger
		, bus& cluster_bus
		, std::shared_ptr<db> db
		, std::chrono::seconds client_timeout)
	: ctl_(ctl)
		, logger_(logger)
		, bus_(cluster_bus)
		, acceptor_(ctl.get_ctx())
		, db_(db)
		, client_timeout_(client_timeout)
		, session_counter_{ 0 }
	{
		CYNG_LOG_INFO(logger_, "[server] created");
	}

	wmbus_server::~wmbus_server()
	{
#ifdef _DEBUG_BROKER_WMBUS
		std::cout << "wmbus_server(~)" << std::endl;
#endif
	}


	void wmbus_server::listen(boost::asio::ip::tcp::endpoint ep) {

		boost::system::error_code ec;
		acceptor_.open(ep.protocol(), ec);
		if (!ec)	acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
		if (!ec)	acceptor_.bind(ep, ec);
		if (!ec)	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
		if (!ec) {
			CYNG_LOG_INFO(logger_, "[server] starts listening at " << ep);
			do_accept();
		}
		else {
			CYNG_LOG_WARNING(logger_, "[server] cannot start listening at "
				<< ep << ": " << ec.message());
		}

	}

	void wmbus_server::do_accept() {

		acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
			if (!ec) {
				CYNG_LOG_INFO(logger_, "[server] new session " << socket.remote_endpoint());

				auto sp = std::shared_ptr<wmbus_session>(new wmbus_session(
					ctl_,
					std::move(socket),
					db_,
					logger_,
					bus_
				), [this](wmbus_session* s) {

					//
					//	update session counter
					//
					--session_counter_;
					CYNG_LOG_TRACE(logger_, "[server] session(s) running: " << session_counter_);

					//
					//	remove session
					//
					delete s;
					});

				if (sp) {

					//
					//	start session
					//
					sp->start(client_timeout_);

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
				CYNG_LOG_WARNING(logger_, "[server] stopped: " << ec.message());
			}
		});

	}

}


