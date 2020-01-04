/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_BRIDGE_H
#define NODE_IPT_SEGW_BRIDGE_H

#include <smf/sml/intrinsics/obis.h>

#include <cyng/store/db.h>
#include <cyng/log.h>

namespace cyng
{
	namespace async
	{
		class mux;
	}
}

namespace node
{
	/**
	 * Manage communication between cache and permanent storage.
	 * This is a singleton. Only one instance is allowed.
	 */
	class storage;
	class cache;
	class lmn;
	class bridge
	{
		friend class lmn;

	public:
		static bridge& get_instance(cyng::async::mux& mux, cyng::logging::log_ptr, cache&, storage&);

	private:
		bridge(cyng::async::mux& mux, cyng::logging::log_ptr, cache&, storage&);
		~bridge() = default;
		bridge(const bridge&) = delete;
		bridge& operator=(const bridge&) = delete;

	public:
		/**
		 * log power return message
		 */
		void power_return();

		void generate_op_log(sml::obis peer
			, std::uint32_t evt
			, std::string target
			, std::uint8_t nr
			, std::string details);

	private:
		/**
		 *	Preload cache with configuration data
		 *	from database.
		 */
		void load_configuration();
		void load_devices_mbus();
		void load_data_collectors();

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
