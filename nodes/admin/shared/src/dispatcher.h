/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_DISPATCHER_H
#define NODE_HTTP_DISPATCHER_H

#include <smf/http/srv/cm_interface.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/vm/controller.h>

namespace node 
{
	class dispatcher
	{

		struct tbl_channel_rel
		{
			tbl_channel_rel(std::string, std::string, std::string);

			const std::string table_;
			const std::string channel_;
			const std::string counter_;

			bool is_empty() const;
			bool has_counter() const;
		};

	public:
		dispatcher(cyng::logging::log_ptr, connection_manager_interface&);

		void register_this(cyng::controller&);

		/**
		 *	subscribe to database
		 */
		void subscribe(cyng::store::db&);

		/**
		 * Update a channel with a specific size/count information. Mostly table size information.
		 */
		void update_channel(std::string const& channel, std::size_t size);

		/** 
		 * Subscribe a table and start initial upload
		 */
		void subscribe_channel(cyng::store::db&, std::string const& channel, boost::uuids::uuid tag);

		/**
		 * Update request from ws
		 */
		void pull(cyng::store::db&, std::string const& channel, boost::uuids::uuid tag);

	private:
		void sig_ins(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::table::data_type const&
			, std::uint64_t
			, boost::uuids::uuid);
		void sig_insert(cyng::table::record const&, std::string);
		void sig_delete(cyng::table::key_type const& key, std::string channel);
		void sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
		void sig_clear(std::string channel);
		void sig_clr(cyng::store::table const*, boost::uuids::uuid);
		void sig_mod(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::attr_t const&
			, std::uint64_t
			, boost::uuids::uuid);

		void subscribe(cyng::store::db&, std::string table, std::string const& channel, boost::uuids::uuid tag);
		void subscribe_with_loop(cyng::store::db&, std::string table, std::string const& channel, boost::uuids::uuid tag);
		void subscribe_TGUIUser(cyng::store::db&, std::string const& channel, boost::uuids::uuid tag);
		void display_loading_icon(boost::uuids::uuid tag, bool, std::string const&);
		void display_loading_level(boost::uuids::uuid tag, std::size_t, std::string const&);

		void subscribe_table_count(cyng::store::db&, std::string const& channel, std::string const& table, boost::uuids::uuid tag);
		void subscribe_web(cyng::store::db&, std::string const& channel, boost::uuids::uuid tag);
		void subscribe_web(cyng::store::db&, std::string const& channel, boost::uuids::uuid tag, std::string);

		void store_relation(cyng::context& ctx);

		void update_sys_cpu_usage_total(cyng::store::db&, std::string const&, boost::uuids::uuid);
		void update_sys_cpu_count(std::string const&, boost::uuids::uuid);
		void update_sys_mem_virtual_total(std::string const&, boost::uuids::uuid);
		void update_sys_mem_virtual_used(std::string const&, boost::uuids::uuid);
		void update_sys_mem_virtual_stat(std::string const&, boost::uuids::uuid);

		void res_proxy_gateway(cyng::context& ctx);
		void res_proxy_job(cyng::context& ctx);
		void res_attention_code(cyng::context& ctx);
		void http_move(cyng::context& ctx);

		/**
		 * Search vector with table/channel relations for a table
		 */
		tbl_channel_rel find_rel_by_table(std::string) const;
		tbl_channel_rel find_rel_by_channel(std::string) const;
		tbl_channel_rel find_rel_by_counter(std::string) const;

	private:
		cyng::logging::log_ptr logger_;
		connection_manager_interface & connection_manager_;

		const std::vector< tbl_channel_rel >	rel_;
	};
}

#endif
 