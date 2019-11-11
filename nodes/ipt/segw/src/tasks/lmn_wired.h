/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_LMN_WIRED_H
#define NODE_SEGW_TASK_LMN_WIRED_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

//#include <boost/filesystem.hpp>

namespace node
{
	/**
	 * control serial I/O
	 */
	class lmn_wired
	{
	public:
		//	[0] static switch ON/OFF
		using msg_0 = std::tuple<bool>;

		using signatures_t = std::tuple<msg_0>;

	public:
		lmn_wired(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::string port
			, std::uint8_t databits
			, std::string parity
			, std::string flow_control
			, std::string stopbits
			, std::uint32_t speed
			, std::size_t);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - static switch ON/OFF
		 *
		 */
		cyng::continuation process(bool);



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
		std::uint8_t databits_;
		std::string parity_;
		std::string flow_control_;
		std::string stopbits_;
		std::uint32_t speed_;
		std::size_t const tid_;

	};

}

#endif