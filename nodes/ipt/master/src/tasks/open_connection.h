/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_OPEN_CONNECTION_H
#define NODE_IP_MASTER_TASK_OPEN_CONNECTION_H

#include <smf/cluster/bus.h>
#include <smf/ipt/defs.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>
#include <boost/predef.h>	//	requires Boost 1.55

namespace node
{

	class open_connection
	{
	public:
		using msg_0 = std::tuple<ipt::response_type>;
		using signatures_t = std::tuple<msg_0>;

	public:
		open_connection(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::string number
			, std::chrono::seconds timeout);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(ipt::response_type);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< ipt device
		const std::string number_;
		const std::chrono::seconds timeout_;
		const std::chrono::system_clock::time_point start_;
		bool is_waiting_;
	};
	
}

#endif
