/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_CSV_TASK_CLUSTER_H
#define NODE_CSV_TASK_CLUSTER_H

#include <smf/cluster/bus.h>
#include <smf/cluster/config.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>

namespace node
{

	class cluster
	{
	public:
		using msg_0 = std::tuple<cyng::version>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		cluster(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::uuids::uuid tag
			, std::string language
			, cluster_config_t const& cfg_cluster
			, cyng::param_map_t const& cfg_db
			, cyng::param_map_t const& cfg_clock_day
			, cyng::param_map_t const& cfg_clock_hour
			, cyng::param_map_t const& cfg_clock_month
			, cyng::param_map_t const& cfg_trigger);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(cyng::version const&);

		/**
		 * @brief slot [1]
		 *
		 * reconnect
		 */
		cyng::continuation process();

	private:
        void connect();
        void reconfigure(cyng::context& ctx);
        void reconfigure_impl();
        void start_sub_tasks();
		void stop_sub_tasks();

		void make_public();

	private:
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
        cluster_redundancy const config_;
		cyng::param_map_t const cfg_db_;
		cyng::param_map_t const cfg_clock_day_;
		cyng::param_map_t const cfg_clock_hour_;
		cyng::param_map_t const cfg_clock_month_;
		std::chrono::minutes const offset_;
		std::chrono::minutes const frame_;
		std::string const format_;
		std::string const language_;

		std::size_t profile_15_min_tsk_
			, profile_60_min_tsk_
			, profile_24_h_tsk_
			, storage_task_;

	};	
}

#endif
