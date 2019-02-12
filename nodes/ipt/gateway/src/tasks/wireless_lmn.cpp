/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "wireless_lmn.h"

namespace node
{

	wireless_LMN::wireless_LMN(cyng::async::base_task* btp
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

	cyng::continuation wireless_LMN::run()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void wireless_LMN::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

}

