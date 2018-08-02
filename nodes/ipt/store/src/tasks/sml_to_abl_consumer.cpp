/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_to_abl_consumer.h"
#include <smf/sml/defs.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	sml_abl_consumer::sml_abl_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	cyng::continuation sml_abl_consumer::run()
	{
		CYNG_LOG_INFO(logger_, "sml_abl_consumer is running");

		return cyng::continuation::TASK_CONTINUE;
	}


	void sml_abl_consumer::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_abl_consumer::process(std::uint64_t line
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

	cyng::continuation sml_abl_consumer::process(std::uint64_t line
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
			<< sml::messages::name(code));
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_abl_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);
		return cyng::continuation::TASK_CONTINUE;
	}

}
