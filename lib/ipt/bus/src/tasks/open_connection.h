/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_OPEN_CONNECTION_H
#define NODE_IPT_BUS_TASK_OPEN_CONNECTION_H

#include <smf/ipt/bus.h>

namespace node
{

	class open_connection
	{
	public:
		using msg_0 = std::tuple<ipt::sequence_type, bool>;
		using msg_1 = std::tuple<ipt::sequence_type>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		open_connection(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::string const&
			, std::chrono::seconds timeout
			, std::size_t retries
			, ipt::bus_interface&);
		cyng::continuation run();
		void stop();

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
		const std::string number_;
		const std::chrono::seconds timeout_;
		std::size_t retries_;
		ipt::bus_interface& bus_;
		ipt::sequence_type seq_;
	};
	
}

#endif
