/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/server.h>
#include <session.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	server::server(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, cyng::logger logger
		, std::string const& account
		, std::string const& pwd
		, std::string const& salt
		, std::chrono::seconds monitor)
	: sigs_{ 
		std::bind(&server::start, this, std::placeholders::_1),
		std::bind(&server::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, tag_(tag)
		, logger_(logger)
		, account_(account)
		, pwd_(pwd)
		, salt_(salt)
		, monitor_(monitor)
		, acceptor_(ctl.get_ctx())
		, session_counter_{ 0 }
		, fabric_(ctl)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("start", 0);
		}

		CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");

	}

	server::~server()
	{
#ifdef _DEBUG_MAIN
		std::cout << "server(~)" << std::endl;
#endif
	}


	void server::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")");
	}

	void server::start(boost::asio::ip::tcp::endpoint ep)
	{
		boost::system::error_code ec;
		acceptor_.open(ep.protocol(), ec);
		if (!ec)	acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
		if (!ec)	acceptor_.bind(ep, ec);
		if (!ec)	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
		if (!ec) {
			CYNG_LOG_INFO(logger_, "server starts listening at "	<< ep);
			do_accept();
		}
		else {
			CYNG_LOG_WARNING(logger_, "server cannot start listening at "
				<< ep << ": " << ec.message());
		}
	}

	void server::do_accept() {
		acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
			if (!ec) {
				CYNG_LOG_INFO(logger_, "new session " << socket.remote_endpoint());

				//std::function<void(std::string)> f = std::bind(&server::pty_connect, this, std::placeholders::_1);
				//auto vm = fabric_.create_proxy(f);	//	ok

				auto sp = std::shared_ptr<session>(new session(
					std::move(socket),
					this,
					logger_
				), [this](session* s) {

					//
					//	update session counter
					//
					--session_counter_;
					CYNG_LOG_TRACE(logger_, "session(s) running: " << session_counter_);

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
				CYNG_LOG_WARNING(logger_, "server stopped: " << ec.message());
			}
		});
	}

	void server::pty_connect(std::string msisdn) {
		CYNG_LOG_INFO(logger_, "pty connect " << msisdn);
	}

}

