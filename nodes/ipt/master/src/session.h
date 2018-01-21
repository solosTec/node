/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_SESSION_H
#define NODE_IPT_MASTER_SESSION_H

#include <smf/cluster/bus.h>
#include <smf/ipt/parser.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>

namespace node 
{
	namespace ipt
	{
		class connection;
		class server;
		class session
		{
			friend class connection;
			friend class server;

		public:
			session(cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, bus::shared_type
				, boost::uuids::uuid tag
				, scramble_key const& sk
				, std::uint16_t watchdog
				, std::chrono::seconds const& timeout);

			session(session const&) = delete;
			session& operator=(session const&) = delete;

		private:
			void ipt_req_login_public(cyng::context& ctx);
			void ipt_req_login_scrambled(cyng::context& ctx);
			void ipt_req_open_push_channel(cyng::context& ctx);
			void ipt_req_transmit_data(cyng::context& ctx);
			void ipt_res_watchdog(cyng::context& ctx);

			void client_res_login(std::uint64_t, bool, std::string msg);

		private:
			cyng::async::mux& mux_;
			cyng::logging::log_ptr logger_;
			bus::shared_type bus_;	//!< cluster bus
			cyng::controller vm_;
			const scramble_key sk_;
			const std::uint16_t watchdog_;
			const std::chrono::seconds timeout_;

			/**
			 * Parser for binary cyng data stream (from cluster members)
			 */
			parser 	parser_;

		};
	}
}

#endif

