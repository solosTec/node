/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "write_json.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	write_json::write_json(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::string root_dir
		, std::string root_name
		, std::string endocing)
	: base_(*btp)
		, logger_(logger)
		, root_dir_(root_dir)
		, root_name_(root_name)
		, endocing_(endocing)
		, parser_([this](cyng::vector_t&& prg) {
			cyng::vector_t r = std::move(prg);
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> process "
				//<< channel_
				//<< ':'
				//<< source_
				<< ' '
				<< cyng::io::to_str(r));
	}, true)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	void write_json::run()
	{
		CYNG_LOG_INFO(logger_, "write_json is running");
	}

	void write_json::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation write_json::process(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> processing "
			<< data.size()
			<< " bytes");

		parser_.read(data.begin(), data.end());
		return cyng::continuation::TASK_CONTINUE;
	}
}
