/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sync.h>

#include <smf/cluster/bus.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	sync::sync(cyng::channel_weak wp
		, cyng::controller& ctl
		, bus& cluster_bus
		, cyng::store& cache
		, cyng::logger logger)
	: sigs_{
		std::bind(&sync::start, this, std::placeholders::_1),
		std::bind(&sync::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, ctl_(ctl)
		, cluster_bus_(cluster_bus)
		, logger_(logger)
		, store_(cache)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("start", 0);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
		}

	}

	sync::~sync()
	{
#ifdef _DEBUG_SETUP
		std::cout << "sync(~)" << std::endl;
#endif
	}

	void sync::start(std::string table_name) {
		CYNG_LOG_INFO(logger_, "task [" << channel_.lock()->get_name() << "] sync");
		//cluster_bus_.req_subscribe(table_name);
	}

	void sync::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "task [" << channel_.lock()->get_name() << "] stopped");
	}
}


