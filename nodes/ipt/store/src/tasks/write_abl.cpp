/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "write_abl.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	write_abl::write_abl(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
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
	}, true, false)	//	verbose, no log instructions
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	cyng::continuation write_abl::run()
	{
		CYNG_LOG_INFO(logger_, "write_abl is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void write_abl::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation write_abl::process(cyng::buffer_t const& data)
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
