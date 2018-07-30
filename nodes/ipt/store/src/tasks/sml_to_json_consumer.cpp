/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "sml_to_json_consumer.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	sml_json_consumer::sml_json_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
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

	cyng::continuation sml_json_consumer::run()
	{
		CYNG_LOG_INFO(logger_, "sml_json_consumer is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_json_consumer::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_json_consumer::process(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::buffer_t const& data)
	{
		const std::uint64_t line = (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);
		return cyng::continuation::TASK_CONTINUE;
	}

}
