/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/storage_xml.h>

#include <smf/cluster/bus.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	storage_xml::storage_xml(cyng::channel_weak wp
		, cyng::controller& ctl
		, bus& cluster_bus
		, cyng::store& cache
		, cyng::logger logger
		, cyng::param_map_t&& cfg)
	: sigs_{
			std::bind(&storage_xml::open, this),
			std::bind(&storage_xml::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, ctl_(ctl)
		, cluster_bus_(cluster_bus)
		, logger_(logger)
		, store_(cache)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("open", 0);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
		}
	}

	storage_xml::~storage_xml()
	{
#ifdef _DEBUG_SETUP
		std::cout << "storage_xml(~)" << std::endl;
#endif
	}

	void storage_xml::open() {

	}

	void storage_xml::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "task [" << channel_.lock()->get_name() << "] stopped");
	}


}


