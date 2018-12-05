/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_WATCHDOG_H
#define NODE_IPT_BUS_TASK_WATCHDOG_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>

namespace node
{

	class watchdog
	{
	public:
		using signatures_t = std::tuple<>;

	public:
		watchdog(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::uint16_t watchdog);
		cyng::continuation run();
		void stop();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< ipt device
		//const std::string name_;
		const std::uint16_t watchdog_;
	};
	
}

#endif
