/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_SETUP_TASK_CLUSTER_H
#define NODE_SETUP_TASK_CLUSTER_H

#include <smf/cluster/bus.h>
#include <smf/cluster/config.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>

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
			, cyng::store::db& cache
			, std::size_t
			, cluster_config_t const& cfg);
		void run();
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
		void db_insert(cyng::context& ctx);
		void db_modify_by_attr(cyng::context& ctx);
		void task_resume(cyng::context& ctx);
		void reconfigure(cyng::context& ctx);
		void reconfigure_impl();
		void create_cache();

	private:
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		const std::size_t storage_tsk_;
		const cluster_config_t	config_;
		std::size_t master_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;
	};
	
}

#endif