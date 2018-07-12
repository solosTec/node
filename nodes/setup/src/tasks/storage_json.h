/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_SETUP_TASK_STORAGE_JSON_H
#define NODE_SETUP_TASK_STORAGE_JSON_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>

namespace node
{
	class storage_json
	{
	public:
		using msg_0 = std::tuple<int, std::string>;
		//using msg_1 = std::tuple<boost::uuids::uuid>;
		//using signatures_t = std::tuple<msg_0>;
		using signatures_t = std::tuple<>;

	public:
		storage_json(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db& cache
			, cyng::param_map_t);
		cyng::continuation run();
		void stop();
		cyng::continuation process(int i, std::string name);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

	};
	
}

#endif