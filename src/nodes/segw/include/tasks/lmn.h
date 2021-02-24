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
			 std::function<void()>
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

		 void set_options(std::string const&);

	 private:
		 signatures_t sigs_;
		 std::weak_ptr<cyng::channel> channel_;
		 cyng::logger logger_;
		 cfg_lmn cfg_;

		 /**
		  * serial port
		  */
		 boost::asio::serial_port port_;

	 };
}

#endif