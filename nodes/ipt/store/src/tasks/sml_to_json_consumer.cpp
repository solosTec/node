/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_to_json_consumer.h"
#include <smf/sml/defs.h>
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
		, boost::filesystem::path root_dir
		, std::string prefix
		, std::string suffix)
	: base_(*btp)
		, logger_(logger)
		, root_dir_(root_dir)
		, prefix_(prefix)
		, suffix_(suffix)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation sml_json_consumer::run()
	{
		CYNG_LOG_INFO(logger_, "sml_json_consumer is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_json_consumer::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_json_consumer::process(std::uint64_t line
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

	cyng::continuation sml_json_consumer::process(std::uint64_t line
		, std::uint16_t code
		, cyng::tuple_t msg)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " line "
			<< line
			<< " received: "
			<< sml::messages::name_from_value(code));
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_json_consumer::process(std::uint64_t line)
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
	cyng::continuation sml_json_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		return cyng::continuation::TASK_CONTINUE;
	}

}
