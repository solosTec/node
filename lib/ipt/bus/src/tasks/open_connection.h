/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_BUS_TASK_OPEN_CONNECTION_H
#define NODE_IPT_BUS_TASK_OPEN_CONNECTION_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

namespace node
{

	class open_connection
	{
	public:
		using msg_0 = std::tuple<bool>;
		using signatures_t = std::tuple<msg_0>;

	public:
		open_connection(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller& vm
			, std::string const&
			, std::chrono::seconds timeout
			, std::size_t retries);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * connection open response
		 */
		cyng::continuation process(bool);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< ipt device
		const std::string number_;
		const std::chrono::seconds timeout_;
		std::size_t retries_;
	};
	
}

//#if BOOST_COMP_GNUC
//namespace cyng {
//	namespace async {
//
//		//
//		//	initialize static slot names
//		//
//		template <>
//		std::map<std::string, std::size_t> cyng::async::task<node::open_connection>::slot_names_;
//    }
//}
//#endif

#endif
