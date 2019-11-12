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
		//	[0] static switch ON/OFF
		using msg_0 = std::tuple<bool>;

		using signatures_t = std::tuple<msg_0>;

	public:
		lmn_port(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::chrono::seconds
			, std::string port
			, std::uint8_t databits
			, std::string parity
			, std::string flow_control
			, std::string stopbits
			, std::uint32_t speed
			, std::size_t tid);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - static switch ON/OFF
		 *
		 */
		cyng::continuation process(bool);

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
		boost::asio::serial_port_base::character_size databits_;
		boost::asio::serial_port_base::parity parity_;
		boost::asio::serial_port_base::flow_control flow_control_;
		boost::asio::serial_port_base::stop_bits stopbits_;
		boost::asio::serial_port_base::baud_rate baud_rate_;
		std::size_t const tid_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer_;

	};

}

#endif