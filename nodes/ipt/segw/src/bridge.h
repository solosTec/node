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
#include <cyng/async/task_fwd.h>

namespace node
{
	namespace ipt {
		class network;
	}
	class storage;
	class cache;
	class bridge

	/**
	 * Manage communication between cache and permanent storage.
	 * This is a singleton. Only one instance is allowed.
	 */
	{
		friend class ipt::network;

	public:
		static bridge& get_instance(cyng::logging::log_ptr
			, cyng::async::mux& mux
			, cache&
			, storage&);

	private:
		bridge(cyng::logging::log_ptr
			, cyng::async::mux& mux
			, cache&
			, storage&);
		~bridge() = default;
		bridge(const bridge&) = delete;
		bridge& operator=(const bridge&) = delete;

	public:
		void generate_op_log(sml::obis peer
			, std::uint32_t evt
			, std::string target
			, std::uint8_t nr
			, std::string details);

		/**
		 * finalize startup process
		 */
		void finalize();

		/**
		 * check the level of data in the data collectors
		 */
		void shrink();

	private:
		/**
		 * log power return message
		 */
		void power_return();

		/**
		 *	Preload cache with configuration data
		 *	from database.
		 */
		void load_configuration();
		void load_devices_mbus();
		void load_data_collectors();
		void load_data_mirror();
		void load_iec_devices();
		void load_privileges();

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
		void sig_trx(cyng::store::trx_type);

		void start_task_clock();
		void start_task_obislog();
		void start_task_limiter();
		void start_task_gpio();
		void start_task_readout();

		void sig_ins_cfg(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::table::data_type const&
			, std::uint64_t
			, boost::uuids::uuid);
		void sig_del_cfg(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
		void sig_clr_cfg(cyng::store::table const*, boost::uuids::uuid);
		void sig_mod_cfg(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::attr_t const&
			, std::uint64_t
			, boost::uuids::uuid);

	private:
		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * task manager
		 */
		cyng::async::mux& mux_;

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
