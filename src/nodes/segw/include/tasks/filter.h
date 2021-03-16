/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_FILTER_H
#define SMF_SEGW_TASK_FILTER_H

#include <cfg.h>
#include <config/cfg_lmn.h>
#include <config/cfg_blocklist.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/task/task_fwd.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>

namespace smf
{
	/**
	 * filter for wireless M-Bus data
	 */
	class filter
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(cyng::buffer_t)>,
			std::function<void(std::string)>		//	reset_target_channels
		>;

	public:
		filter(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger
			, cfg&
			, lmn_type);

	private:
		void stop(cyng::eod);

		/**
		 * incoming raw data from serial interface
		 */
		void receive(cyng::buffer_t);
		void reset_target_channels(std::string);

		void check(mbus::radio::header const& h, cyng::buffer_t const& data);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;

		cyng::controller& ctl_;

		/**
		 * global logger
		 */
		cyng::logger logger_;

		cfg_blocklist	cfg_blocklist_;

		/**
		 * parser for wireless M-Bus data
		 */
		mbus::radio::parser parser_;

		std::vector<cyng::channel_ptr>	targets_;

	};
}

#endif