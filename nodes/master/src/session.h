/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_SESSION_H
#define NODE_MASTER_SESSION_H

#include "client.h"
#include "cluster.h"
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/vm/controller.h>
#include <cyng/io/parser/parser.h>
#include <cyng/io/serializer/serialize.hpp>
#include <boost/process/child.hpp>

namespace node 
{
	class connection;
	class watchdog;
	class cache;
	class session
	{
		friend class connection;
		friend class client;
		friend class cluster;
		friend class watchdog;

	public:
		session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache&
			, std::string const& account
			, std::string const& pwd
			, boost::uuids::uuid stag);

		session(session const&) = delete;
		session& operator=(session const&) = delete;

		/**
		 * @return session specific hash based in internal tag
		 */
		std::size_t hash() const noexcept;

		/**
		 * Fast session shutdown. No cleanup.
		 * Session watchdog calls this method during service
		 * shutdown.
		 *
		 * @param obj reference object of this session to keep session alive
		 */
		void stop(cyng::object obj);

	private:
		void bus_req_login(cyng::context& ctx);
		void bus_req_login_impl(cyng::context& ctx
			, cyng::version
			, std::string const&
			, std::string const&
			, boost::uuids::uuid		//	[3] session tag
			, std::string const&			//	[4] class
			, std::chrono::minutes	//	[5] delta
			, std::chrono::system_clock::time_point	//	[6] timestamp
			, bool					//	[7] autologin
			, std::uint32_t			//	[8] group
			, boost::asio::ip::tcp::endpoint	//	[9] remote ep
			, std::string			//	[10] platform
			, std::int64_t);		//	boost::process::pid_t
		void bus_req_subscribe(cyng::context& ctx);
		void bus_req_unsubscribe(cyng::context& ctx);
		void bus_start_watchdog(cyng::context& ctx);
		void res_watchdog(cyng::context& ctx);
		void bus_req_stop_client_impl(cyng::context& ctx);


		void cleanup(cyng::context& ctx);
		void time_series(cyng::context& ctx);
		void bus_insert_msg(cyng::context& ctx);
		void bus_req_push_data(cyng::context& ctx);
		void bus_insert_lora_uplink(cyng::context& ctx);
		void bus_insert_wmbus_uplink(cyng::context& ctx);

		cyng::vector_t reply(std::chrono::system_clock::time_point, bool);

		void sig_ins(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::table::data_type const&
			, std::uint64_t
			, boost::uuids::uuid);
		void sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
		void sig_clr(cyng::store::table const*, boost::uuids::uuid);
		void sig_mod(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::attr_t const&
			, std::uint64_t
			, boost::uuids::uuid);

		void stop_cb(cyng::vm&, cyng::object);

		/**
		 * remove this cluster session from cluster table
		 * @return node type
		 */
		std::string cleanup_cluster_table(boost::uuids::uuid tag);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cache& cache_;
		cyng::controller vm_;
		
		/**
		 * Parser for binary cyng data stream (from cluster members)
		 */
		cyng::parser 	parser_;
		
		std::string const account_;
		std::string const pwd_;

		/**
		 * cluster bus sequence
		 */
		std::uint64_t seq_;

		/**
		 * separate implementation of client logic
		 */
		client	client_;

		/**
		 * separate implementation of cluster logic
		 */
		cluster	cluster_;

		/**
		 * table subscriptions
		 */
		cyng::store::subscriptions_t	subscriptions_;

		/**
		 * watchdog task id
		 */
		std::size_t tsk_watchdog_;

		/**
		 * group id
		 */
		std::uint32_t group_;

		/**
		 * Cluster tag of this session.
		 * This tag is unique for each node.
		 */
		boost::uuids::uuid cluster_tag_;

	};

	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache&
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag);
}

#include <cyng/intrinsics/traits.hpp>
namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::session>
		{
			using type = node::session;
			using tag = std::integral_constant<std::size_t,
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_SESSION)
#else
				PREDEF_SESSION
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "session";
#else
			const static char name[];
#endif
		};

		template <>
		struct reverse_type <
#if defined(__CPP_SUPPORT_N2347)
			static_cast<std::size_t>(traits::predef_type_code::PREDEF_SESSION)
#else
			PREDEF_SESSION
#endif
		>
		{
			using type = node::session;
		};
	}

}

#include <functional>
#include <boost/functional/hash.hpp>

namespace std
{
	template<>
	struct hash<node::session>
	{
		inline size_t operator()(node::session const& s) const noexcept
		{
			return s.hash();
		}
	};
	template<>
	struct equal_to<node::session>
	{
		using result_type = bool;
		using first_argument_type = node::session;
		using second_argument_type = node::session;

		inline bool operator()(node::session const& s1, node::session const& s2) const noexcept
		{
			return s1.hash() == s2.hash();
		}
	};
	template<>
	struct less<node::session>
	{
		using result_type = bool;
		using first_argument_type = node::session;
		using second_argument_type = node::session;

		inline bool operator()(node::session const& s1, node::session const& s2) const noexcept
		{
			return s1.hash() < s2.hash();
		}
	};

}



#endif

