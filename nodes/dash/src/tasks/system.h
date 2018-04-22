/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_DASH_TASK_SYSTEM_H
#define NODE_DASH_TASK_SYSTEM_H

#include <smf/cluster/bus.h>
#include <smf/cluster/config.h>
#include <smf/http/srv/server.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	class system
	{
	public:
		using msg_0 = std::tuple<>;
		using signatures_t = std::tuple<msg_0>;

	public:
		system(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db&
			, boost::uuids::uuid);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * dummy
		 */
		cyng::continuation process();

	private:

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

		const boost::uuids::uuid tag_;

	};
	
}

#endif