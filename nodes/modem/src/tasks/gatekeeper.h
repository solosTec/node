/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_MODEM_TASK_GATEKEEPER_H
#define NODE_MODEM_TASK_GATEKEEPER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>


namespace node
{

	class gatekeeper
	{
	public:
		using msg_0 = std::tuple<bool>;
		using signatures_t = std::tuple<msg_0>;

	public:
		gatekeeper(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, boost::uuids::uuid tag
			, std::chrono::seconds timeout);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(bool);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;
		const boost::uuids::uuid tag_;
		const std::chrono::seconds timeout_;
		const std::chrono::system_clock::time_point start_;
		bool success_;
		bool is_waiting_;
	};
	
}

#endif