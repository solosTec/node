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

	cyng::continuation storage_log::run()
	{
		CYNG_LOG_INFO(logger_, "storage_log is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_log::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_log::process(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::buffer_t const& data)
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
			//
			//	ToDo: start a task with a type according to specified protocol.
			//	Then send data to this task.
			//

			//	create a new parser
			if (boost::algorithm::equals(protocol, "SML"))
			{
				auto res = lines_.emplace(std::piecewise_construct,
					std::forward_as_tuple(line),
					std::forward_as_tuple(std::bind(&storage_log::cb, this, channel, source, target, protocol, std::placeholders::_1)
						, false
						, true));

				CYNG_LOG_TRACE(logger_, "processing line "
					<< std::hex
					<< line
					<< ": "
					<< std::dec
					<< source
					<< '/'
					<< channel
					<< " with new task "
				);

				if (res.second)
				{
					//res.first->second.vm_.register_function("stop.writer", 1, std::bind(&storage_log::stop_writer, this, std::placeholders::_1));
					res.first->second.read(data.begin(), data.end());
				}
				else
				{
					CYNG_LOG_FATAL(logger_, "startup processor for line "
						<< channel
						<< ':'
						<< source
						<< ':'
						<< target
						<< " failed");
				}
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "protocol "
					<< protocol
					<< " not supported yet");

			}
		}
		else
		{

			//	take the existing task
			pos->second.read(data.begin(), data.end());
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_log::cb(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::vector_t&& prg)
	{
		CYNG_LOG_DEBUG(logger_, "db processor "
			<< channel
			<< ':'
			<< source
			<< ':'
			<< target
			<< " - "
			<< protocol
			<< " - "
			<< prg.size()
			<< " instructions");

	}

}
