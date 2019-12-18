/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_BRIDGE_H
#define NODE_IPT_SEGW_BRIDGE_H

namespace cyng
{
	namespace async
	{
		class mux;
	}
}

#include <cyng/store/db.h>
#include <cyng/log.h>

namespace node
{
	/**
	 * Manage communication between cache and permanent storage
	 */
	class storage;
	class cache;
	class lmn;
	class bridge
	{
		friend class lmn;

	public:
		bridge(cyng::async::mux& mux, cyng::logging::log_ptr, cache&, storage&);

		/**
		 * log power return message
		 */
		void power_return();

	private:
		/**
		 *	Preload cache with configuration data
		 *	from database.
		 */
		void load_configuration();
		void load_devices_mbus();

		/**
		 * Tracking all changes in the cache tables.
		 */
		void connect_to_cache();

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

		void start_task_obislog(cyng::async::mux& mux);
		void start_task_gpio(cyng::async::mux& mux);
		void start_task_readout(cyng::async::mux& mux);

	private:
		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cache& cache_;

		/**
		 * permanent storage
		 */
		storage& storage_;
	};
}
#endif
