/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_DASH_TASK_OUI_H
#define NODE_DASH_TASK_OUI_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/store/db.h>

namespace node
{
	class oui
	{
	public:
		using msg_0 = std::tuple<>;
		using signatures_t = std::tuple<msg_0>;

	public:
		oui(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db&
			, std::string file_name
			, boost::uuids::uuid);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * dummy
		 */
		cyng::continuation process();

	private:
		void load();
		void log_progress(std::size_t counter, std::chrono::system_clock::time_point now);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

		std::string const file_name_;
		boost::uuids::uuid const tag_;

	};
	
}

#endif