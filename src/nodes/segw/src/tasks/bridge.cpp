/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/bridge.h>
#include <storage_functions.h>
#include <smf/obis/defs.h>

#include <cyng/task/channel.h>
#include <cyng/log/record.h>
#include <cyng/db/storage.h>
#include <cyng/parse/buffer.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

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
		, cfg_(logger, cache_)
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
		init_lmn_ports();

		//
		//	virtual meter
		//
		CYNG_LOG_INFO(logger_, "initialize: virtual meter");

		//
		//	IP-T client
		//	connect to IP-T server
		//
		CYNG_LOG_INFO(logger_, "initialize: IP-T client");
	}

	void bridge::init_data_cache() {

		auto const data = get_store_meta_data();
		for (auto const& m : data) {

			CYNG_LOG_TRACE(logger_, "create table: " << m.get_name());
			cache_.create_table(m);
		}
	}

	void bridge::load_config_data() {

		//
		//	Preload cache with configuration data.
		//	Preload must be done before we connect to the cache. Otherwise
		//	we would receive our own changes.
		//
		load_configuration();
		auto tpl = cfg_.get_obj("language-code");
		//load_devices_mbus();
		//load_data_collectors();
		//load_data_mirror();
		//load_iec_devices();
		//load_privileges();

	}

	void bridge::load_configuration() {

		cache_.access([&](cyng::table* cfg) {

			cyng::db::storage s(db_);
			s.loop(get_table_cfg(), [&](cyng::record const& rec)->bool {

#ifdef _DEBUG_SEGW
				//CYNG_LOG_DEBUG(logger_, rec.to_tuple());
#endif
				auto const path = rec.value("path", "");
				auto const type = rec.value<std::uint16_t>("type", 15u);
				auto const val = rec.value("val", "");

				try {

					//
					//	restore original data type from string
					//
					auto obj = cyng::restore(val, type);

#ifdef _DEBUG_SEGW
					CYNG_LOG_DEBUG(logger_, "load - "
						<< path
						<< " = "
						<< obj);
#endif
					if (boost::algorithm::equals(path, "tag")) {
						//	set system tag
						cfg_.tag_ = cyng::value_cast(obj, boost::uuids::nil_uuid());
					}
					else if (boost::algorithm::equals(path, cyng::to_str(OBIS_SERVER_ID))) {
						//	init server ID in cache
						cfg_.id_ = cyng::to_buffer(val);
					}
					else {

						//
						//	insert value
						//
						cfg->merge(rec.key()
							, cyng::data_generator(obj)
							, 1u	//	only needed for insert operations
							, boost::uuids::nil_uuid());	//	tag not available yet
					}

				}
				catch (std::exception const& ex) {

					CYNG_LOG_ERROR(logger_, "cannot load "
						<< path
						<< ": "
						<< ex.what());
				}

				return true;
			});

		}, cyng::access::write("cfg"));

		//
		//	debug
		//
		//cache_.access([&](cyng::table* cfg) {
		//	cfg->loop([](cyng::record const& rec, std::size_t idx)->bool {
		//		std::cout << rec.to_tuple() << std::endl;
		//		return true;
		//		});
		//}, cyng::access::write("cfg"));
	}

	void bridge::init_lmn_ports() {

	}

}