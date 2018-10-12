/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_REGISTER_TARGET_H
#define NODE_IPT_BUS_TASK_REGISTER_TARGET_H

#include <smf/ipt/bus.h>
//#include <smf/ipt/defs.h>

namespace node
{

	class register_target
	{
	public:
		using msg_0 = std::tuple<ipt::sequence_type, bool, std::uint32_t>;
		using msg_1 = std::tuple<ipt::sequence_type>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		register_target(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::string const& name
			, std::uint16_t packet_size
			, std::uint8_t window_size
			, std::chrono::seconds timeout
			, ipt::bus_interface& bus);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push target register response
		 *
		 * @param seq ipt sequence
		 * @param success success flag
		 * @param channe ipt channel
		 */
		cyng::continuation process(ipt::sequence_type seq
			, bool success
			, std::uint32_t channel);

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
		const std::string name_;
		const std::uint16_t packet_size_;
		const std::uint8_t window_size_;
		const std::chrono::seconds timeout_;
		bool is_waiting_;	//!< task state
		ipt::bus_interface& bus_;
		ipt::sequence_type seq_;
	};
	
}

#endif
