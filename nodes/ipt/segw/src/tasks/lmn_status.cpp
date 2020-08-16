/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "lmn_status.h"
#include "../cache.h"

#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/compatibility.h>

#include <iostream>

namespace node
{
	lmn_status::lmn_status(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cache& cfg
		, sml::status_bits sb)
	: base_(*btp) 
		, logger_(logger)
		, cache_(cfg)
		, status_bits_(sb)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::get_status_name(status_bits_));
	}

	cyng::continuation lmn_status::run()
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

	void lmn_status::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::get_status_name(status_bits_)
			<< " is stopped");
	}

	cyng::continuation lmn_status::process(std::size_t bytes, std::size_t msg)
	{

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::get_status_name(status_bits_)
			<< " received msg #"
			<< msg
			<< " with " 
			<< cyng::bytes_to_str(bytes));

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation lmn_status::process(bool b)
	{
		//	sml::STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE
		cache_.set_status_word(status_bits_, b);

		if (b) {
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< sml::get_status_name(status_bits_)
				<< " available");
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< sml::get_status_name(status_bits_)
				<< " lost");
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}
}

