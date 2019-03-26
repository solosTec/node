/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_CLOSE_CONNECTION_H
#define NODE_IPT_BUS_TASK_CLOSE_CONNECTION_H

#include <smf/ipt/bus.h>

namespace node
{

	class close_connection
	{
	public:
		using msg_0 = std::tuple<ipt::sequence_type, bool>;
		using msg_1 = std::tuple<ipt::sequence_type>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		close_connection(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::chrono::seconds timeout
			, ipt::bus_interface& bus);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * connection open response
		 */
		cyng::continuation process(ipt::sequence_type, bool);

		/**
		 * @brief slot [1]
		 *
		 * update sequence
		 */
		cyng::continuation process(ipt::sequence_type);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< ipt device
		const std::chrono::seconds timeout_;
		bool is_waiting_;	//!< task state
		ipt::bus_interface& bus_;
		ipt::sequence_type seq_;
	};
	
}

#endif
