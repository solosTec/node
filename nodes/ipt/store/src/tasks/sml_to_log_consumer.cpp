/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_to_log_consumer.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	sml_log_consumer::sml_log_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation sml_log_consumer::run()
	{
		CYNG_LOG_INFO(logger_, "sml_log_consumer is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_log_consumer::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_log_consumer::process(std::uint64_t line
		, std::string target)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " create line "
			<< line
			<< ':'
			<< target);

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_log_consumer::process(std::uint64_t line
		, std::uint16_t code
		, std::size_t idx
		, cyng::tuple_t msg)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " line "
			<< line
			<< " received #"
			<< idx
			<< ':'
			<< sml::messages::name(code));
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_log_consumer::process(std::uint64_t line)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);
		return cyng::continuation::TASK_CONTINUE;
	}

	//	EOM
	cyng::continuation sml_log_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_log_consumer::cb(std::uint32_t channel
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
