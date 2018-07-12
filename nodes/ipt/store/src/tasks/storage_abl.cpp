/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_abl.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	storage_abl::storage_abl(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	cyng::continuation storage_abl::run()
	{
		CYNG_LOG_INFO(logger_, "storage_abl is running");

		return cyng::continuation::TASK_CONTINUE;
	}


	void storage_abl::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_abl::process(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::buffer_t const& data)
	{
		const std::uint64_t line = (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);
		return cyng::continuation::TASK_CONTINUE;
	}
}
