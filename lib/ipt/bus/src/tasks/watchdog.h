/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_WATCHDOG_H
#define NODE_IPT_BUS_TASK_WATCHDOG_H

#include <smf/ipt/defs.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

namespace node
{

	class watchdog
	{
	public:
		using msg_0 = std::tuple<ipt::sequence_type, bool, std::uint32_t, std::size_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		watchdog(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::string const& name
			, std::uint16_t packet_size
			, std::uint8_t window_size
			, std::chrono::seconds timeout);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * connection open response
		 */
		cyng::continuation process(ipt::sequence_type, bool, std::uint32_t, std::size_t);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< ipt device
		const std::string name_;
		const std::uint16_t packet_size_;
		const std::uint8_t window_size_;
		const std::chrono::seconds timeout_;
		bool is_waiting_;	//!< task state
	};
	
}

#endif
