/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IEC_62056_TASK_CLIENT_H
#define NODE_IEC_62056_TASK_CLIENT_H

//#include <smf/cluster/bus.h>
//#include <smf/cluster/config.h>
//#include "../sync_db.h"
//#include "../server.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>

namespace node
{

	class client
	{
	public:
		//using msg_0 = std::tuple<cyng::version>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_1>;

	public:
		client(cyng::async::base_task* bt
			, cyng::logging::log_ptr);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		//cyng::continuation process(cyng::version const&);

		/**
		 * @brief slot [1]
		 *
		 * reconnect
		 */
		cyng::continuation process();

	private:

	private:
		/**
		 * communication bus to master
		 */
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		//cyng::store::db cache_;

	};
	
}

#endif
