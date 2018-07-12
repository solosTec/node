/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_json.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	storage_json::storage_json(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::string root_dir
		, std::string prefix
		, std::string suffix)
	: base_(*btp)
		, logger_(logger)
		, root_dir_(root_dir)
		, prefix_(prefix)
		, suffix_(suffix)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	cyng::continuation storage_json::run()
	{
		CYNG_LOG_INFO(logger_, "storage_json is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_json::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_json::process(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::buffer_t const& data)
	{
		const std::uint64_t line = (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);
		return cyng::continuation::TASK_CONTINUE;
	}

}
