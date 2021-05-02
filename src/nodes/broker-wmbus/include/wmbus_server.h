/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_WMBUS_SERVER_H
#define SMF_WMBUS_SERVER_H

#include <db.h>
#include <smf/cluster/bus.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/controller.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class wmbus_server
	{
		using blocklist_type = std::vector<boost::asio::ip::address>;

	public:
		wmbus_server(cyng::controller& ctl
			, cyng::logger
			, bus&
			, std::shared_ptr<db>
			, std::chrono::seconds client_timeout);
		~wmbus_server();

		void stop(cyng::eod);
		void listen(boost::asio::ip::tcp::endpoint);

	private:
		/**
		 * incoming connection
		 */
		void do_accept();


	private:
		cyng::controller& ctl_;
		cyng::logger logger_;
		bus& bus_;
		std::shared_ptr<db> db_;
		std::chrono::seconds const client_timeout_;
		boost::asio::ip::tcp::acceptor acceptor_;
		std::uint64_t session_counter_;

	};

}

#endif
