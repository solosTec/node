/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/server.h>
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
			//do_accept();
		}
		else {
			CYNG_LOG_WARNING(logger_, "server cannot start listening at "
				<< ep << ": " << ec.message());
		}

	}


}


