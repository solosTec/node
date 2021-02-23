/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/cluster/bus.h>

#include <cyng/log/record.h>

namespace smf {

	bus::bus(cyng::logger logger, toggle cfg, cyng::channel_weak wp)
	: logger_(logger)
		, cfg_(cfg)
		, wchannel_(wp)
	{}

	void bus::start() {
		auto const srv = cfg_.get();
		CYNG_LOG_INFO(logger_, "cluster::connect(" << srv << ")");

	}

	void bus::stop() {

	}

}

