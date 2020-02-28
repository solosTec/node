/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_TRANSFER_DATA_H
#define NODE_IPT_BUS_TASK_TRANSFER_DATA_H

#include <smf/ipt/bus.h>

namespace node
{

	class transfer_data
	{
	public:
		using msg_0 = std::tuple<bool, std::uint32_t, std::uint32_t, std::uint8_t, std::uint8_t>;
		using msg_1 = std::tuple<ipt::sequence_type>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		transfer_data(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::uint32_t channel
			, std::uint32_t source
			, cyng::buffer_t const& data
			, std::uint16_t packet_size
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
			, std::uint8_t status
			, std::uint8_t block);

		/**
		 * @brief slot [1]
		 *
		 * update sequence
		 */
		cyng::continuation process(ipt::sequence_type);

	private:
		bool send_next_block();
		bool is_complete() const;

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< ipt device
		std::uint32_t const channel_;
		std::uint32_t const source_;
		std::vector<cyng::buffer_t> chuncks_;
		ipt::bus_interface& bus_;
		std::size_t const task_;
		ipt::sequence_type seq_;
		std::uint8_t block_;
	};
	
	std::vector<cyng::buffer_t> make_chuncks(cyng::buffer_t const&, std::uint16_t);
	std::size_t get_total_size(std::vector<cyng::buffer_t> const& v);
}

#endif
