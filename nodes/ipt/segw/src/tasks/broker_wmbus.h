/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_BROKER_WMBUS_H
#define NODE_SEGW_TASK_BROKER_WMBUS_H

//#include <smf/mbus/parser.h>
#include "../cfg_wmbus.h"


#include <cyng/log.h>
#include <cyng/async/mux.h>

#include <cyng/compatibility/file_system.hpp>

namespace cyng
{
	class controller;
}

namespace node
{
	/**
	 * receiver and parser for wireless mbus data
	 */
	class cache;
	class broker_wmbus
	{
	public:
		//	[0] receive data
		using msg_0 = std::tuple<cyng::buffer_t, std::size_t>;
		//	[1] status (open/closed)
		using msg_1 = std::tuple<bool>;

		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		broker_wmbus(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, cache& cfg);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - receive data
		 *
		 */
		cyng::continuation process(cyng::buffer_t, std::size_t);

		/**
		 * @brief slot [1] - status (open/closed)
		 *
		 */
		cyng::continuation process(bool);

		/**
		 * connect to server
		 */
		bool connect();


	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * configuration management
		 */
		cfg_wmbus cfg_;

		/**
		 * connection socket
		 */
		//boost::asio::ip::tcp::socket socket_;
		boost::asio::ip::tcp::iostream stream_;

	};

}

#endif