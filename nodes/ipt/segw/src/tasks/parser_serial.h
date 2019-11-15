/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_PARSER_SERIAL_H
#define NODE_SEGW_TASK_PARSER_SERIAL_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

#include <boost/filesystem.hpp>

namespace cyng
{
	class controller;
}
namespace node
{
	/**
	 * receiver and parser for serial data (RS485) aka IEC
	 */
	class parser_serial
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;


		using signatures_t = std::tuple<msg_0>;

	public:
		parser_serial(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - write an entry
		 *
		 */
		cyng::continuation process();

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

	};

}

#endif