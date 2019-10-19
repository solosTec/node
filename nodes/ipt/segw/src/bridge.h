/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_BRIDGE_H
#define NODE_IPT_SEGW_BRIDGE_H

#include <cyng/store/db.h>

namespace node
{
	/**
	 * Manage communication between cache and permanent storage
	 */
	class storage;
	class cache;
	class bridge
	{

	public:
		bridge(cache&, storage&);

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

	private:

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
