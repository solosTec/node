/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_READOUT_H
#define NODE_SEGW_TASK_READOUT_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/store/store_fwd.h>
#include <cyng/table/table_fwd.h>
#include <smf/sml/intrinsics/obis.h>

namespace node
{
	/**
	 * Check cache tables "_Readout" and "_ReadoutData" regularily
	 * to distribute readout data into SQL tables "TStorage_8181C78610FF", ...
	 */
	class storage;
	class cache;
	class readout
	{
	public:
		//	[0] static switch ON/OFF
		using msg_0 = std::tuple<cyng::buffer_t>;

		using signatures_t = std::tuple<msg_0>;

	public:
		readout(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, storage&
			, std::chrono::seconds);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - distribute readout data
		 * of specified meter.
		 *
		 */
		cyng::continuation process(cyng::buffer_t);

	private:
		void distribute();
		void distribute(cyng::buffer_t const& srv);

		void profile_1_minute(cyng::store::table const* tbl_data
			, cyng::store::table const* tbl_mirror
			, cyng::table::record const& rec_collector
			, cyng::table::record const& rec_meta
			, cyng::table::key_list_t const& result);
		void profile_15_minute(cyng::store::table const* tbl_data
			, cyng::store::table const* tbl_mirror
			, cyng::table::record const& rec_collector
			, cyng::table::record const& rec_meta
			, cyng::table::key_list_t const& result);

		/**
		 * 60 minute profile
		 */
		void profile_60_min(cyng::store::table const* tbl_data
			, cyng::store::table const* tbl_mirror
			, cyng::table::record const& rec_collector
			, cyng::table::record const& rec_meta
			, cyng::table::key_list_t const& result);

		void profile_24_hour(cyng::store::table const* tbl_data
			, cyng::store::table const* tbl_mirror
			, cyng::table::record const& rec_collector
			, cyng::table::record const& rec_meta
			, cyng::table::key_list_t const& result);

		/**
		 * Configure a data collector for the specified profile.
		 */
		void auto_configure_data_collector(std::map<cyng::buffer_t, sml::obis_path_t>&, sml::obis profile);

		/**
		 * Remove all inactive collectors
		 */
		static void cleanup_collectors(cyng::store::table const*
			, cyng::table::key_list_t&);

	private:
		cyng::async::base_task& base_;

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

		/**
		 * interval time to backup data to SQL database
		 */
		std::chrono::seconds const interval_;
	};

}

#endif