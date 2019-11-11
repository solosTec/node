/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "lmn_wireless.h"
//#include <fstream>
#include <iostream>

namespace node
{
	lmn_wireless::lmn_wireless(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::string port
		, std::uint8_t databits
		, std::string parity
		, std::string flow_control
		, std::string stopbits
		, std::uint32_t speed
		, std::size_t tid)
	: base_(*btp) 
		, logger_(logger)
		, port_(btp->mux_.get_io_service(), port)
		, databits_(databits)
		, parity_(parity)
		, flow_control_(flow_control)
		, stopbits_(stopbits)
		, speed_(speed)
		, tid_(tid)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		boost::asio::serial_port_base::baud_rate baud_rate(speed_);
		port_.set_option(baud_rate);
	}

	cyng::continuation lmn_wireless::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif

		return cyng::continuation::TASK_CONTINUE;
	}

	void lmn_wireless::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation lmn_wireless::process(bool on)
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

