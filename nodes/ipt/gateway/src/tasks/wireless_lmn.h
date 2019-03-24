/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_GATEWAY_TASK_WIRELESS_LMN_H
#define NODE_GATEWAY_TASK_WIRELESS_LMN_H


#include <smf/ipt/bus.h>
#include <smf/mbus/parser.h>

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/async/mux.h>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

namespace node
{
	/**
	 * wireless M-Bus
	 */
	class wireless_LMN
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		using signatures_t = std::tuple<>;

	public:
		wireless_LMN(cyng::async::base_task* btp
			, cyng::logging::log_ptr
			, cyng::store::db& config_db
			, cyng::controller& vm
			, std::string port
			, std::uint8_t databits
			, std::string parity
			, std::string flow_control
			, std::string stopbits
			, std::uint32_t speed
			, std::size_t tid);

		cyng::continuation run();
		void stop();

	private:
		void do_read();

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * configuration db
		 */
		cyng::store::db& config_db_;

		/**
		 * execution engine
		 */
		cyng::controller& vm_;

		/**
		 * serial port
		 */
		boost::asio::serial_port port_;

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_;

		/**
		 * GPIO control
		 */
		std::size_t const task_gpio_;

		/**
		 * M-Bus parser
		 */
		wmbus::parser parser_;
	};
}

#endif