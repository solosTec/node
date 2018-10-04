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
			, cluster_config_t const& cfg_cluster
			, cyng::param_map_t const& cfg_db
			, cyng::param_map_t const& cfg_clock_day
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


	private:
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
        const cluster_redundancy config_;
		const cyng::param_map_t cfg_db_;
		const cyng::param_map_t cfg_clock_day_;
		const cyng::param_map_t cfg_clock_month_;
		const cyng::param_map_t cfg_trigger_;
		std::size_t profile_15_min_tsk_, profile_24_h_tsk_, storage_task_;

	};	
}

#endif
