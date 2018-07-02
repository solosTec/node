/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "storage_xml.h"
#include <cyng/async/task/base_task.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

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
		, hit_list_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> initialized");
	}

	void storage_xml::run()
	{
		if (!hit_list_.empty())
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop "
				<< hit_list_.size()
				<< " processor(s)");
			for (auto line : hit_list_)
			{
				//
				//	call destructor
				//
				lines_.erase(line);
			}
			hit_list_.clear();
		}
		else
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< lines_.size()
				<< " processor(s) online");

		}
		base_.suspend(std::chrono::seconds(16));
	}

	void storage_xml::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_xml::process(std::uint32_t channel
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
		//	select/create a SML processor
		//
		const auto pos = lines_.find(line);
		if (pos == lines_.end())
		{
			//
			//	create new processor
			//
			auto res = lines_.emplace(std::piecewise_construct,
				std::forward_as_tuple(line),
				std::forward_as_tuple(base_.mux_.get_io_service()
					, logger_
					, rng_()
					, root_dir_
					, root_name_
					, endocing_
					, channel
					, source
					, target));

			if (res.second)
			{
				//
				//	This is a workaround for problems with nested strands
				//
				res.first->second.vm_.register_function("stop.writer", 1, std::bind(&storage_xml::stop_writer, this, std::placeholders::_1));
				res.first->second.parse(data);
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
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> processing line "
				<< std::dec
				<< source
				<< ':'
				<< channel
				<< " with processor "
				<< pos->second.vm_.tag());

			pos->second.parse(data);
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_xml::stop_writer(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

		for (auto pos = lines_.begin(); pos != lines_.end(); ++pos)
		{
			if (pos->second.vm_.tag() == tag)
			{
				CYNG_LOG_INFO(logger_, "remove xml processor " << tag);

				//
				//	stop VM (async)
				//
				pos->second.stop();

				//
				//	add to hitlist and remove later
				//
				hit_list_.push_back(pos->first);
				return;
			}
		}
		CYNG_LOG_ERROR(logger_, "xml processor " << tag << " not found");
	}
}
