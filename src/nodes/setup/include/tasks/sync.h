/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SETUP_TASK_SYNC_H
#define SMF_SETUP_TASK_SYNC_H


#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/store/db.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class bus;
	class sync
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(std::string)>,
			std::function<void(cyng::eod)>
		>;

	public:
		sync(std::weak_ptr<cyng::channel>
			, cyng::controller&
			, bus& cluster_bus
			, cyng::store& cache
			, cyng::logger logger);
		~sync();


		void stop(cyng::eod);

	private:
		void start(std::string table_name);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		bus& cluster_bus_;
		cyng::logger logger_;
		cyng::store& store_;
	};

}

#endif
