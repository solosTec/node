/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_REDIRECTOR_H
#define SMF_SEGW_TASK_REDIRECTOR_H

#include <cfg.h>
#include <config/cfg_lmn.h>
#include <config/cfg_gpio.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

#include <boost/asio/serial_port.hpp>

namespace smf {

	/**
	 * Receive data from LMN/filter and forward this data
	 * to the listener session
	 */
	class redirector
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(std::string)>,	//	start
			std::function<void(cyng::buffer_t)>
		>;

		using cb_f = std::function<void(cyng::buffer_t)>;

	public:
		redirector(std::weak_ptr<cyng::channel>
			, cyng::registry& reg
			, cyng::logger
			, cb_f);

	private:
		void stop(cyng::eod);
		void start(std::string);

		/**
		 * incoming data from LMN/filter
		 */
		void receive(cyng::buffer_t);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::registry& registry_;
		cyng::logger logger_;
		cb_f cb_;

	};
}

#endif