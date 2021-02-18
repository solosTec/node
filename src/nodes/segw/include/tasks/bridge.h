/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_BRIDGE_H
#define SMF_SEGW_TASK_BRIDGE_H

#include <storage_functions.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/store/db.h>

namespace cyng {
	template <typename T >
	class task;

	class channel;
}
namespace smf {

	 /**
	  * manage segw lifetime
	  */
	 class bridge
	 {
		 template <typename T >
		 friend class cyng::task;

		 using signatures_t = std::tuple<
			 std::function<void(cyng::eod)>,
			 std::function<void()>
		 >;

	 public:
		 bridge(std::weak_ptr<cyng::channel>
			 , cyng::logger
			 , cyng::db::session);

	 private:
		 void stop(cyng::eod);
		 void start();

		 void init_data_cache();
		 void load_config_data();

	 private:
		 signatures_t sigs_;
		 std::weak_ptr<cyng::channel> channel_;
		 cyng::logger logger_;
		 cyng::db::session db_;
		 cyng::store cache_;

	 };
}

#endif