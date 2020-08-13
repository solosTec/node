/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_PARSER_CP210x_H
#define NODE_SEGW_TASK_PARSER_CP210x_H

#include <smf/hci/parser.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>


namespace cyng
{
	class controller;
}
namespace node
{
	/**
	 * receiver and parser for serial data (RS485) aka IEC
	 */
	class parser_CP210x
	{
	public:
		//	[0] receive data
		using msg_0 = std::tuple<cyng::buffer_t, std::size_t>;

		using signatures_t = std::tuple<msg_0>;

	public:
		parser_CP210x(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, cyng::async::task_list_t const& receiver);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - receive data
		 *
		 */
		cyng::continuation process(cyng::buffer_t, std::size_t);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * Receiver task for payloads
		 */
		cyng::async::task_list_t const receiver_;

		/**
		 * HCI parser to unwrap payload data
		 */
		hci::parser parser_;

	};

}

#endif