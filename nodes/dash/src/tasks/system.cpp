/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "system.h"
//#include <smf/cluster/generator.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
//#include <cyng/vm/generator.h>
#include <cyng/table/meta.hpp>
//#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/dom/reader.h>
#include <cyng/sys/cpu.h>
//#include <cyng/sys/memory.h>
//#include <cyng/json.h>

//#include <boost/uuid/nil_generator.hpp>
//#include <boost/uuid/string_generator.hpp>
//#include <boost/algorithm/string/predicate.hpp>

namespace node
{
	system::system(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::uuids::uuid tag)
	: base_(*btp)
		, logger_(logger)
		, cache_(db)
		, tag_(tag)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running");

	}

	void system::run()
	{	
		const auto load = cyng::sys::get_total_cpu_load();
		//CYNG_LOG_DEBUG(logger_, "CPU load " 
		//	<< load 
		//	<< "%");

		cache_.modify("*Config", cyng::table::key_generator("cpu:load"), cyng::param_factory("value", load), tag_);

		//
		//	measure CPU load every second
		//
		base_.suspend(std::chrono::seconds(1));
	}

	void system::stop()
	{
		CYNG_LOG_INFO(logger_, "system task is stopped");
	}

	cyng::continuation system::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}


}
