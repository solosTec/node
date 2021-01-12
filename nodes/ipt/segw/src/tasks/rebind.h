/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_REBIND_H
#define NODE_SEGW_TASK_REBIND_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * Allow ro rebind the listener (of an server) to a new endpoint
	 */
	class rebind
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<boost::asio::ip::tcp::endpoint>;

		using signatures_t = std::tuple<msg_0>;

	public:
		rebind(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::asio::ip::tcp::acceptor&);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - rebind listener
		 *
		 */
		cyng::continuation process(boost::asio::ip::tcp::endpoint);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * tcp/ip listener
		 */
		boost::asio::ip::tcp::acceptor& acceptor_;
	};

}

#endif