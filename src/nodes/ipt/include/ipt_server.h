/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SERVER_H
#define SMF_IPT_SERVER_H

#include <smf/ipt/scramble_key.h>
#include <smf/cluster/bus.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class ipt_session;
	class ipt_server
	{
		friend class ipt_session;
		using blocklist_type = std::vector<boost::asio::ip::address>;

	public:
		ipt_server(boost::asio::io_context&
			, boost::uuids::uuid tag
			, cyng::logger
			, ipt::scramble_key const& sk
			, std::chrono::minutes watchdog
			, std::chrono::seconds timeout
			, bus&);
		~ipt_server();

		void stop(cyng::eod);
		void listen(boost::asio::ip::tcp::endpoint);

	private:
		/**
		 * incoming connection
		 */
		void do_accept();


	private:
		boost::uuids::uuid const tag_;
		cyng::logger logger_;

		ipt::scramble_key const sk_;
		std::chrono::minutes const watchdog_;
		std::chrono::seconds const timeout_;

		bus& cluster_bus_;

		boost::asio::ip::tcp::acceptor acceptor_;
		std::uint64_t session_counter_;

	};

}

#endif
