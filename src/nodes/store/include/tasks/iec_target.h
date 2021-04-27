/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_IEC_TARGET_H
#define SMF_STORE_TASK_IEC_TARGET_H

#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/task/controller.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class iec_target
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(std::string)>,
			std::function<void(std::uint32_t, std::uint32_t, cyng::buffer_t data, std::string)>,
			std::function<void(cyng::eod)>
		>;

	public:
		iec_target(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger logger
			, ipt::bus&);
		~iec_target();

		void stop(cyng::eod);

	private:
		void register_target(std::string);
		void receive(std::uint32_t, std::uint32_t, cyng::buffer_t data, std::string);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		cyng::logger logger_;
		ipt::bus& bus_;
	};

}

#endif
