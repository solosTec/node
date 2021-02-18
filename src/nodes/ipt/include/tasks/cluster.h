/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_TASK_CLUSTER_H
#define SMF_IPT_TASK_CLUSTER_H

#include <smf/cluster/bus.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace cyng {
	template <typename T >
	class task;

	class channel;
}
namespace smf {

	class cluster
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(void)>,
			std::function<void(int)>,
			std::function<void(int, std::string, float)>,
			std::function<void(int)>,
			std::function<void(cyng::eod)>
		>;

	public:
		cluster(std::weak_ptr<cyng::channel>
			, boost::uuids::uuid tag
			, cyng::logger
			, toggle cluster_cfg);
		~cluster();

		void connect();
		void status_check(int);
		void login(bool);	//	login callback
		void demo3(int);

		void stop(cyng::eod);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		bus	bus_;
	};

}

#endif
