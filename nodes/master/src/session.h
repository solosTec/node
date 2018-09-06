/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_SESSION_H
#define NODE_MASTER_SESSION_H

#include "client.h"
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
	class client;
	class watchdog;
	class session
	{
		friend class connection;
		friend class client;
		friend class watchdog;

	public:
		session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid mtag
			, cyng::store::db&
			, std::string const& account
			, std::string const& pwd
			, boost::uuids::uuid stag
			, std::chrono::seconds monitor
			, std::atomic<std::uint64_t>& global_configuration
			, boost::filesystem::path);

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
			, std::string				//	[10] platform
			, boost::process::pid_t);
		void bus_req_subscribe(cyng::context& ctx);
		void bus_req_unsubscribe(cyng::context& ctx);
		void bus_start_watchdog(cyng::context& ctx);
		void res_watchdog(cyng::context& ctx);
		void bus_req_stop_client_impl(cyng::context& ctx);

		void bus_req_reboot_client(cyng::context& ctx);
		void bus_req_query_srv_visible(cyng::context& ctx);
		void bus_res_query_srv_visible(cyng::context& ctx);
		void bus_req_query_srv_active(cyng::context& ctx);
		void bus_res_query_srv_active(cyng::context& ctx);
		void bus_req_query_firmware(cyng::context& ctx);
		void bus_res_query_firmware(cyng::context& ctx);

		void cleanup(cyng::context& ctx);
		void bus_insert_msg(cyng::context& ctx);
		void bus_req_push_data(cyng::context& ctx);

		cyng::vector_t reply(std::chrono::system_clock::time_point, bool);

		void client_req_login(cyng::context& ctx);
		void client_req_close(cyng::context& ctx);
		void client_res_close(cyng::context& ctx);

		void client_req_open_push_channel(cyng::context& ctx);
		void client_res_open_push_channel(cyng::context& ctx);
		void client_req_close_push_channel(cyng::context& ctx);
		void client_req_register_push_target(cyng::context& ctx);
		void client_req_deregister_push_target(cyng::context& ctx);
		void client_req_open_connection(cyng::context& ctx);
		void client_res_open_connection(cyng::context& ctx);
		void client_req_close_connection(cyng::context& ctx);
		void client_res_close_connection(cyng::context& ctx);
		void client_req_transfer_pushdata(cyng::context& ctx);

		void client_req_transmit_data(cyng::context& ctx);
		void client_inc_throughput(cyng::context& ctx);
		void client_update_attr(cyng::context& ctx);

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

		std::tuple<session const*, cyng::table::record, cyng::buffer_t, boost::uuids::uuid> find_peer(cyng::table::key_type const& key_session
			, const cyng::store::table* tbl_session
			, const cyng::store::table* tbl_gw);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		boost::uuids::uuid mtag_;	// master tag
		cyng::store::db& db_;
		cyng::controller vm_;
		
		/**
		 * Parser for binary cyng data stream (from cluster members)
		 */
		cyng::parser 	parser_;
		
		const std::string account_;
		const std::string pwd_;
		const std::chrono::seconds cluster_monitor_;


		/**
		 * cluster bus sequence
		 */
		std::uint64_t seq_;

		/**
		 * separate implementation of client logic
		 */
		client	client_;

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
	};

	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid mtag
		, cyng::store::db&
		, std::string const& account
		, std::string const& pwd
		, boost::uuids::uuid stag
		, std::chrono::seconds monitor //	cluster watchdog
		, std::atomic<std::uint64_t>& global_configuration
		, boost::filesystem::path);
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
			using tag = std::integral_constant<std::size_t, PREDEF_SESSION>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "session";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SESSION >
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
}



#endif

