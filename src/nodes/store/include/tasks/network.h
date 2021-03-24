/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_NETWORK_H
#define SMF_STORE_TASK_NETWORK_H

#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/task/controller.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class network
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(void)>,
			//std::function<void(boost::asio::ip::tcp::endpoint)>,
			std::function<void(cyng::eod)>
		>;

	public:
		network(std::weak_ptr<cyng::channel>
			, cyng::controller&
			, cyng::logger logger
			, boost::uuids::uuid tag);
		~network();

		void stop(cyng::eod);

	private:
		void connect();

		//
		//	bus interface
		//

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;
		std::unique_ptr<ipt::bus>	bus_;
	};

}

#endif
