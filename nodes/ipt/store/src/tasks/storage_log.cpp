/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_log.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	storage_log::storage_log(cyng::async::base_task* btp
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

	void storage_log::run()
	{
		CYNG_LOG_INFO(logger_, "storage_log is running");
	}

	void storage_log::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_log::process(std::uint32_t channel, std::uint32_t source, std::string const& target, cyng::buffer_t const& data)
	{
		//
		//	create the line ID by combining source and channel into one 64 bit integer
		//
		const std::uint64_t line = (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);
		//
		//	select/create a SML processor task
		//
		const auto pos = lines_.find(line);
		if (pos == lines_.end())
		{
			//	create a new task
			//auto r = broker_.get_factory().start < task_process_db >(logger_
			//	, broker_.get_tag()
			//	, pool_
			//	, source
			//	, channel);

			CYNG_LOG_TRACE(logger_, "processing line "
				<< std::hex
				<< line
				<< ": "
				<< std::dec
				<< source
				<< '/'
				<< channel
				<< " with new task "
				//<< r.second
			);

			//	insert into task list
			//lines_[line] = r.second;

			//	take the new task
			//broker_.get_ruler().send<cyy::buffer_t>(r.second, 0, cyy::buffer_t(data));

		}
		else
		{
			CYNG_LOG_TRACE(logger_, "processing line "
				<< std::hex
				<< line
				<< ": "
				<< std::dec
				<< source
				<< '/'
				<< channel
				<< " with task "
				<< pos->second);

			//	take the existing task
			//broker_.get_ruler().send<cyy::buffer_t>(pos->second, 0, cyy::buffer_t(data));
		}

		return cyng::continuation::TASK_CONTINUE;
	}
}
