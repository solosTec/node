/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_TASK_CLUSTER_H
#define SMF_DASH_TASK_CLUSTER_H

#include <smf/cluster/bus.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/vm/mesh.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class cluster : private bus_interface
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(void)>,
			std::function<void(int)>,
			//std::function<void(int, std::string, float)>,
			std::function<void(cyng::eod)>
		>;

	public:
		cluster(std::weak_ptr<cyng::channel>
			, cyng::controller&
			, boost::uuids::uuid tag
			, std::string const& node_name
			, cyng::logger
			, toggle::server_vec_t&&);
		~cluster();

		void connect();
		void status_check(int);

		void stop(cyng::eod);

	private:
		//
		//	bus interface
		//
		virtual cyng::mesh* get_fabric() override;
		virtual void on_login(bool) override;

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		cyng::mesh fabric_;
		bus	bus_;
	};

}

#endif
