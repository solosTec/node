/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "wired_lmn.h"

namespace node
{
	wired_LMN::wired_LMN(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& config_db
		, cyng::controller& vm
		, std::string port
		, std::uint8_t databits
		, std::uint8_t paritybit
		, std::uint8_t rtscts
		, std::uint8_t stopbits
		, std::uint32_t speed)
	: base_(*btp) 
		, logger_(logger)
		, config_db_(config_db)
		, vm_(vm)
		, port_(port)
		, databits_(databits)
		, paritybit_(paritybit)
		, rtscts_(rtscts)
		, stopbits_(stopbits)
		, speed_(speed)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation wired_LMN::run()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void wired_LMN::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

}

