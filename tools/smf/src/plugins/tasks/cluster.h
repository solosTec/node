/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_JOIN_CLUSTER_H
#define NODE_TOOL_SMF_PLUGIN_JOIN_CLUSTER_H

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
			, boost::uuids::uuid cluster_tag
			, cluster_config_t const& cfg_cls);
		cyng::continuation run();
		void stop(bool shutdown);

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

	private:
		cyng::async::base_task& base_;

		/**
		 * cluster bus - communication to master node
		 */
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;

		/**
		 * cluster configuration
		 */
		cluster_redundancy	const config_;
	};
	
}

#endif