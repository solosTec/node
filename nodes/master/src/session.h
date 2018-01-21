/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_SESSION_H
#define NODE_MASTER_SESSION_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/vm/controller.h>
#include <cyng/io/parser/parser.h>

namespace node 
{
	class connection;
	class session
	{
		friend class connection;

	public:
		session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db&
			, std::string const& account
			, std::string const& pwd
			, std::chrono::seconds const& monitor);

		session(session const&) = delete;
		session& operator=(session const&) = delete;

	private:
		void bus_req_login(cyng::context& ctx);
		void bus_req_subscribe(cyng::context& ctx);
		void bus_req_unsubscribe(cyng::context& ctx);

		cyng::vector_t reply(cyng::vector_t const&, bool);

		void client_req_login(cyng::context& ctx);
		void client_req_open_push_channel(cyng::context& ctx);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;
		cyng::controller vm_;
		const std::string account_;
		const std::string pwd_;
		const std::chrono::seconds monitor_;

		/**
		 * Parser for binary cyng data stream (from cluster members)
		 */
		cyng::parser 	parser_;

	};
}

#endif

