/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/bridge.h>
#include <storage_functions.h>

#include <cyng/task/channel.h>
#include <cyng/log/record.h>

namespace smf {

	bridge::bridge(cyng::channel_weak wp
		, cyng::logger logger
		, cyng::db::session db)
	: sigs_{
		std::bind(&bridge::stop, this, std::placeholders::_1),
		std::bind(&bridge::start, this),
	}	, channel_(wp)
		, logger_(logger)
		, db_(db)
		, cache_()
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("start", 1);
		}

		CYNG_LOG_INFO(logger_, "segw ready");
	}

	void bridge::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "segw stop");
	}

	void bridge::start() {
		CYNG_LOG_INFO(logger_, "segw start");

		//
		//	initialize data cache
		//
		CYNG_LOG_INFO(logger_, "initialize: data cache");
		init_data_cache();

		//
		//	load configuration data from data base
		//
		CYNG_LOG_INFO(logger_, "initialize: configuration data");
		load_config_data();

		//
		//	GPIO
		//
		CYNG_LOG_INFO(logger_, "initialize: GPIO");

		//
		//	connect database to data cache
		//
		CYNG_LOG_INFO(logger_, "initialize: persistent data");

		//
		//	router for SML data
		//
		CYNG_LOG_INFO(logger_, "initialize: SML router");

		//
		//	SML server
		//
		CYNG_LOG_INFO(logger_, "initialize: SML server");

		//
		//	NMS server
		//
		CYNG_LOG_INFO(logger_, "initialize: NMS server");

		//
		//	LMN - open serial ports
		//
		CYNG_LOG_INFO(logger_, "initialize: LMN ports");

		//
		//	virtual meter
		//
		CYNG_LOG_INFO(logger_, "initialize: virtual meter");

	}

	void bridge::init_data_cache() {
		std::vector< cyng::meta_sql > ms = get_sql_meta_data();
		cache_.create_table(cyng::meta_store("demo-1"
			, {
				cyng::column("id", cyng::TC_INT64),
				cyng::column("name", cyng::TC_STRING),
				cyng::column("age", cyng::TC_TIME_POINT)
			}
		, 1));

	}

	void bridge::load_config_data() {

	}

}