/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_LMN_PORT_H
#define NODE_SEGW_TASK_LMN_PORT_H

#include <NODE_project_info.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * control serial port
	 */
	class lmn_port
	{
	public:
		//	[0] send data
		using msg_0 = std::tuple<cyng::buffer_t>;

		//	[1] add/remove receiver
		using msg_1 = std::tuple<std::size_t, bool>;

		//	[2] modify options int
		//	SERIAL_DATABITS
		//	SERIAL_PARITY
		using msg_2 = std::tuple<cyng::buffer_t, std::uint32_t>;

		//	[3] modify options string
		//	SERIAL_FLOW_CONTROL
		//	SERIAL_STOPBITS
		//	SERIAL_SPEED
		using msg_3 = std::tuple<cyng::buffer_t, std::string>;

		using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3>;

	public:
		lmn_port(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::chrono::seconds
			, std::string port
			, boost::asio::serial_port_base::character_size databits
			, boost::asio::serial_port_base::parity parity
			, boost::asio::serial_port_base::flow_control flow_control
			, boost::asio::serial_port_base::stop_bits stopbits
			, boost::asio::serial_port_base::baud_rate speed
			, std::size_t receiver_status
			, cyng::buffer_t&& init);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - send data
		 *
		 */
		cyng::continuation process(cyng::buffer_t);

		/**
		 * @brief slot [1] - add/remove receiver
		 *
		 */
		cyng::continuation process(std::size_t, bool);

		//	@brief slot [2] modify options int
		//	SERIAL_DATABITS
		//	SERIAL_SPEED
		cyng::continuation process(cyng::buffer_t, std::uint32_t);

		//	@brief slot [3] modify options string
		//	SERIAL_FLOW_CONTROL
		//	SERIAL_STOPBITS
		//	SERIAL_PARITY
		cyng::continuation process(cyng::buffer_t, std::string);

	private:
		void do_read();
		void set_all_options();

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * serial port
		 */
		boost::asio::serial_port port_;

		/**
		 * Parameter
		 */
		std::chrono::seconds const monitor_;
		std::string const name_;

		//
		//	initial values
		//
		boost::asio::serial_port_base::character_size databits_;
		boost::asio::serial_port_base::parity parity_;
		boost::asio::serial_port_base::flow_control flow_control_;
		boost::asio::serial_port_base::stop_bits stopbits_;
		boost::asio::serial_port_base::baud_rate baud_rate_;

		cyng::async::task_list_t receiver_data_;
		std::size_t const receiver_status_;
		cyng::buffer_t init_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer_;

		std::size_t msg_counter_;

	};

}

#endif