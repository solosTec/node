/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_LMN_H
#define SMF_SEGW_TASK_LMN_H

#include <cfg.h>
#include <config/cfg_lmn.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

#include <boost/asio/serial_port.hpp>

namespace smf {

	/**
	 * manage segw lifetime
	 */
	class lmn
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void()>,
			std::function<void(cyng::buffer_t)>,
			std::function<void(std::string)>,
			std::function<void(std::uint32_t)>,		//	set_baud_rate
			std::function<void(std::string)>,		//	set_parity
			std::function<void(std::string)>,		//	set_flow_control
			std::function<void(std::string)>,		//	set_stopbits
			std::function<void(std::uint8_t)>		//	set_databits
		>;

	public:
		lmn(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger
			, cfg&
			, lmn_type);

	private:
		void stop(cyng::eod);
		void open();
		void reset_target_channels(std::string);

		void set_options(std::string const&);
		void do_read();
		void do_write(cyng::buffer_t);

		//
		//	communication parameters
		//
		void set_baud_rate(std::uint32_t);
		void set_parity(std::string);
		void set_flow_control(std::string);
		void set_stopbits(std::string);
		void set_databits(std::uint8_t);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		cyng::logger logger_;
		cfg_lmn cfg_;

		/**
		 * serial port
		 */
		boost::asio::serial_port port_;

		/**
		 * input buffer
		 */
		std::array<char, 1024> buffer_;

		std::vector<cyng::channel_ptr>	targets_;

	};
}

#endif