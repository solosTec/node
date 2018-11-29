/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_GATEKEEPER_H
#define NODE_IP_MASTER_TASK_GATEKEEPER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

namespace node
{

	class gatekeeper
	{
	public:
		using msg_0 = std::tuple<bool>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		gatekeeper(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, std::chrono::seconds timeout);
		~gatekeeper();
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(bool);

		/**
		 * @brief slot [1]
		 *
		 * session closed
		 */
		cyng::continuation process();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;
		const std::chrono::seconds timeout_;
		const std::chrono::system_clock::time_point start_;
		bool is_waiting_;
	};
	
}

#if BOOST_COMP_GNUC
namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::gatekeeper>::slot_names_;
    }
}
#endif

#endif
