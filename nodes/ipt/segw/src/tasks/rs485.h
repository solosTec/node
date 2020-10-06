/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_RS485_H
#define NODE_SEGW_TASK_RS485_H

#include "../cfg_mbus.h"
#include "../decoder.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{
	class cache;
	/**
	 * Manage RS 485 interface as wired mbus protocol.
	 * Allow to initialize a wired M-Bus, search for devices, etc
	 */
	class rs485
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;
		using msg_1 = std::tuple<
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[5] checksum
			cyng::buffer_t		//	[6] payload
		>;
		using msg_2 = std::tuple<
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[4] CI-field
			std::uint8_t,		//	[5] checksum
			cyng::buffer_t		//	[6] payload
		>;
		using msg_3 = std::tuple<
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[4] CI-field
			std::uint8_t		//	[5] checksum
		>;

		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3>;

	public:
		rs485(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, cache&
			, std::size_t);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - ack
		 *
		 */
		cyng::continuation process();

		/**
		 * @brief slot [1] - short frame
		 *
		 */
		cyng::continuation process(
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[5] checksum
			cyng::buffer_t		//	[6] payload
		);

		/**
		 * @brief slot [2] - long frame
		 *
		 */
		cyng::continuation process(
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[4] CI-field
			std::uint8_t,		//	[5] checksum
			cyng::buffer_t		//	[6] payload
		);

		/**
		 * @brief slot [3] - ctrl frame
		 *
		 */
		cyng::continuation process(
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[4] CI-field
			std::uint8_t		//	[5] checksum
		);

	private:
		void search();
		void readout();
		void remove_secondary_address();
		void initialize_all_meters();
		std::string get_state() const;

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

		decoder_wired_mbus decoder_mbus_;

		enum class state {
			REMOVE_SECONDARY_ADDRESS,
			INITIALIZE_ALL_METERS,
			SEARCH,
			QUERY,
			READOUT,
		} state_;

		/**
		 *  LMN task
		 */
		std::size_t const tsk_;

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

	/**
	 * select a slave by ID
	 */
	cyng::buffer_t mbus_select_slave_by_id(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t);

	/**
	 * set primary address.
	 * @note: the slave must be selected previously with mbus_select_slave_by_id()
	 */
	cyng::buffer_t mbus_set_address(std::uint8_t);

}

#endif