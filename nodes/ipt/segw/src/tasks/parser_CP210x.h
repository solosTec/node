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
	 * receiver and parser for CP210x host control protocol (HCP)
	 */
	class parser_CP210x
	{
	public:
		//	[0] receive data
		using msg_0 = std::tuple<cyng::buffer_t, std::size_t>;
		//	[1] add/remove receiver
		using msg_1 = std::tuple<std::size_t, bool>;

		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		parser_CP210x(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - receive data
		 *
		 */
		cyng::continuation process(cyng::buffer_t, std::size_t);

		/**
		 * @brief slot [1] - add/remove receiver
		 *
		 */
		cyng::continuation process(std::size_t, bool);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * Receiver task for payloads
		 */
		cyng::async::task_list_t receiver_;

		/**
		 * HCI parser to unwrap payload data
		 */
		hci::parser parser_;

	};

}

#endif