/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <tasks/reflux.h>
#include <cache.h>

#include <cyng/parser/chrono_parser.h>
#if BOOST_OS_LINUX
#include <cyng/sys/rtc.h>
#endif

#include <cyng/io/io_chrono.hpp>
#include <cyng/io/io_bytes.hpp>

namespace node
{
	reflux::reflux(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::function<void(cyng::buffer_t, std::size_t)> cb)
	: base_(*btp)
		, logger_(logger)
		, cb_(cb)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">")

	}

	cyng::continuation reflux::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	start/restart timer
		//
		//base_.suspend(std::chrono::minutes(5));

		return cyng::continuation::TASK_CONTINUE;
	}

	void reflux::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation reflux::process(cyng::buffer_t data, std::size_t msg_counter)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received msg #"
			<< msg_counter
			<< " with "
			<< cyng::bytes_to_str(data.size()));

		cb_(data, msg_counter);

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

