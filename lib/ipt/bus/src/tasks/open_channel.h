/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_OPEN_CHANNEL_H
#define NODE_IPT_BUS_TASK_OPEN_CHANNEL_H

#include <smf/ipt/bus.h>

namespace node
{

	class open_channel
	{
	public:
		using msg_0 = std::tuple<bool, std::uint32_t, std::uint32_t, std::uint16_t, std::uint32_t>;
		using msg_1 = std::tuple<ipt::sequence_type>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		open_channel(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::string const& target
			, std::string const& account
			, std::string const& msisdn
			, std::string const& version
			, std::string const& device
			, std::uint16_t time_out
			, ipt::bus_interface&
			, std::size_t tsk);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * connection open response
		 */
		cyng::continuation process(bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::uint32_t count);

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
		std::string const target_;
		std::string const account_;
		std::string const msisdn_;
		std::string const version_;
		std::string const device_;
		std::uint16_t const timeout_;
		ipt::bus_interface& bus_;
		std::size_t const task_;
		ipt::sequence_type seq_;
		bool waiting_;
	};
	
}

#endif
