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

namespace node 
{
	class dispatcher
	{
	public:
		dispatcher(cyng::logging::log_ptr, connection_manager_interface&);

		/**
		 *	subscribe to database
		 */
		void subscribe(cyng::store::db&);

		/**
		 * Update a channel with a specific size/count information. Mostly table size information.
		 */
		void update_channel(std::string const& channel, std::size_t size);


	private:
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

	private:
		cyng::logging::log_ptr logger_;
		connection_manager_interface & connection_manager_;
	};
}

#endif
