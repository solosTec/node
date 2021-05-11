/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_BROKER_WMBUS_TASK_PUSH_H
#define SMF_BROKER_WMBUS_TASK_PUSH_H

#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/task/controller.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class push
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(void)>,
			std::function<void(cyng::eod)>
		>;

	public:
		push(std::weak_ptr<cyng::channel>
			, cyng::controller&
			, cyng::logger
			, ipt::toggle::server_vec_t&&
			, ipt::push_channel&& pcc);
		~push();


		void stop(cyng::eod);

	private:
		void connect();

		//
		//	bus interface
		//
		void ipt_cmd(ipt::header const&, cyng::buffer_t&&);
		void ipt_stream(cyng::buffer_t&&);
		void auth_state(bool);


	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::logger logger_;
		ipt::toggle::server_vec_t toggle_;
		ipt::push_channel const pcc_;
		ipt::bus	bus_;
	};

}

#endif