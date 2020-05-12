/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SETUP_TASK_SYNC_H
#define NODE_SETUP_TASK_SYNC_H

#include <smf/cluster/bus.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>

namespace node
{
	/**
	 * 1. create table cache
	 * 2. subscribe remote
	 * 3. merge cache
	 * 4. upload data
	 * 5. listening for updates, insertes, etc.
	 */
	class sync
	{
	public:
		using msg_0 = std::tuple<std::string, std::size_t>;
		using msg_1 = std::tuple<std::size_t>;
		using msg_2 = std::tuple<>;	//	disconnect
		using signatures_t = std::tuple<msg_0, msg_1, msg_2>;

	public:
		sync(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, bus::shared_type
			, std::size_t tsk
			, std::string const&
			, cyng::store::db&);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * slot [0] - load cache complete
		 */
		cyng::continuation process(std::string name, std::size_t sync_tsk);

		/**
		 * slot [1] - cache complete, upload data to master
		 */
		cyng::continuation process(std::size_t);

		/**
		 * slot [2] - disconnect from cache
		 */
		cyng::continuation process();

	private:
		void subscribe();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		bus::shared_type bus_;
		const std::size_t tsk_cluster_;	//!< cluster task
		const std::string table_;
		cyng::store::db& cache_;
	};
	
}

#endif