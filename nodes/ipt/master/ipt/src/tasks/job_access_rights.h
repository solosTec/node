/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_JOB_ACCESS_RIGHTS_H
#define NODE_IP_MASTER_TASK_JOB_ACCESS_RIGHTS_H

#include "../proxy_data.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
//#include <cyng/vm/controller.h>

namespace node
{

	class job_access_rights
	{
	public:
		//
		//	empty message map
		//
		using msg_0 = std::tuple<>;
		using signatures_t = std::tuple<msg_0>;

	public:
		job_access_rights(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t tsk_proxy
			, ipt::proxy_data&&);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * activity
		 */
		cyng::continuation process();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		std::size_t const tsk_proxy_;
		ipt::proxy_data const proxy_data_;
	};
	
}

#endif