/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/storage_json.h>
#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	storage_json::storage_json(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, cyng::logger logger
		, cyng::param_map_t&& cfg)
	: sigs_{
		std::bind(&storage_json::open, this),
		std::bind(&storage_json::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("open", 0);
		}

		CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");

	}

	storage_json::~storage_json()
	{
#ifdef _DEBUG_IPT
		std::cout << "cluster(~)" << std::endl;
#endif
	}

	void storage_json::open() {

	}

	void storage_json::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop storage_json task(" << tag_ << ")");
	}


}


