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
#include "../dispatcher.h"
#include "../sync_db.h"

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
			, boost::uuids::uuid tag
			, cluster_config_t const& cfg_cluster
			, cyng::param_map_t cfg_db);
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

		//void create_cache();
		void sync_table(std::string const& name);
		void res_subscribe(cyng::context& ctx);

	private:
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
        const cluster_redundancy config_;
		const cyng::param_map_t cfg_db_;

		cyng::store::db cache_;
		dispatcher dispatcher_;
		db_sync db_sync_;

	};	
}

#endif
