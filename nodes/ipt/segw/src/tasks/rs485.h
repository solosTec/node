/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_RS485_H
#define NODE_SEGW_TASK_RS485_H

#include "../cfg_mbus.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	class cache;
	/**
	 * manage RS 485 interface as wired mbus protocol
	 */
	class rs485
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;

		using signatures_t = std::tuple<msg_0>;

	public:
		rs485(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, std::size_t);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - ack
		 *
		 */
		cyng::continuation process();

	private:
		void search();
		void readout();
		void remove_secondary_address();
		void initialize_all_meters();

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * config database
		 */
		cache& cache_;
		cfg_mbus cfg_;

		/**
		 *  LMN task
		 */
		std::size_t const tsk_;

		enum class state {
			REMOVE_SECONDARY_ADDRESS,
			INITIALIZE_ALL_METERS,
			SEARCH,
			QUERY,
			READOUT,
		} state_;

		/**
		 * Used in state SEARCH to pick up the next
		 * mbus address.
		 */
		std::uint8_t search_idx_;
		bool response_;
	};

	/**
	 * The Check Sum is calculated from the arithmetical sum of the data mentioned above, 
	 * without taking carry digits into account.
	 */
	void mbus_calculate_checksum(cyng::buffer_t&);

	/**
	 * The address field serves to address the recipient in the calling direction,and to identify the sender of information in the
	 * receiving direction.The size of this field is one Byte,and can therefore take values from 0 to 255. The addresses 1 to 250
	 * can be allocated to the individual slaves,up to a maximum of 250.Unconfigured slaves are given the address 0 at
	 * manufacture,and as a rule are allocated one of these addresses when connected to the M-Bus. The addresses254 (FE)
	 * and 255 (FF) are used to transmit information to all participants (Broadcast). With address 255 none of the slaves reply,
	 * and with address 254 all slaves reply with their own addresses.The latter case naturally results in collisions when two or
	 * more slaves are connected,and should only be used for test purposes.The address 253 (FD) indicates that the addressing
	 * has been performed in the Network Layer instead of Data Link Layer,The FD used when using The second level address.
	 * The remaining addresses 251 and 252 have been kept for future applications.
	 */
	cyng::buffer_t mbus_initialize_slave(std::uint8_t);

	/**
	 * Read energy information
	 */
	cyng::buffer_t mbus_read_data_by_primary_address(bool fcb, std::uint8_t);

	cyng::buffer_t mbus_check_secondary_address(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t);
}

#endif