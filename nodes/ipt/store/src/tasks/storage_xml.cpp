/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "storage_xml.h"
#include "write_xml.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	storage_xml::storage_xml(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::string root_dir
		, std::string root_name
		, std::string endocing)
	: base_(*btp)
		, logger_(logger)
		, root_dir_(root_dir)
		, root_name_(root_name)
		, endocing_(endocing)
		, lines_()
		, rng_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	void storage_xml::run()
	{
		CYNG_LOG_INFO(logger_, "storage_xml is running");
	}

	void storage_xml::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_xml::process(std::uint32_t channel, std::uint32_t source, std::string const& target, cyng::buffer_t const& data)
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
			auto tp = cyng::async::make_task<write_xml>(base_.mux_
				, logger_
				, rng_()
				, root_dir_
				, root_name_
				, endocing_
				, channel
				, source
				, target);

			//
			//	provide storage functionality
			//
			cyng::async::task_cast<write_xml>(tp)->vm_.run(cyng::register_function("stop.writer", 1, std::bind(&storage_xml::stop_writer, this, std::placeholders::_1)));

			//	start task
			auto res = cyng::async::start_task_sync(base_.mux_, tp);

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " processing line "
				<< std::dec
				<< source
				<< ':'
				<< channel
				<< " with new task "
				<< res.first
			);

			//	insert into task list
			lines_[line] = res.first;

			//	take the new task
			base_.mux_.send(res.first, 0, cyng::tuple_factory(data));

		}
		else
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " processing line "
				<< std::dec
				<< source
				<< ':'
				<< channel
				<< " with task "
				<< pos->second);

			//	take the existing task
			if (!base_.mux_.send(pos->second, 0, cyng::tuple_factory(data)))
			{
				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< " processing line "
					<< std::dec
					<< source
					<< ':'
					<< channel
					<< " no task "
					<< pos->second);
			}
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_xml::stop_writer(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "stop.writer " << cyng::io::to_str(frame));

		auto tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);

		for (auto pos = lines_.begin(); pos != lines_.end(); ++pos)
		{
			if (pos->second == tsk)
			{
				CYNG_LOG_INFO(logger_, "stop.writer " << tsk);
				lines_.erase(pos);
				//base_.mux_.stop(tsk);
				base_.mux_.send(tsk, 1, cyng::tuple_t{});
				return;
			}
		}
		CYNG_LOG_ERROR(logger_, "stop.writer " << tsk << " not found");
	}
}
