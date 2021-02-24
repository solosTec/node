/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage_functions.h>

#include <tasks/bridge.h>
#include <tasks/lmn.h>
#include <tasks/gpio.h>
#include <tasks/broker.h>

#include <config/cfg_gpio.h>
#include <config/cfg_broker.h>

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
		, cyng::controller& ctl
		, cyng::logger logger
		, cyng::db::session db
		, ipt::toggle::server_vec_t const& tgl)
	: sigs_{
		std::bind(&bridge::stop, this, std::placeholders::_1),
		std::bind(&bridge::start, this),
	}	, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, db_(db)
		, cache_()
		, cfg_(logger, cache_)
		, fabric_(ctl)
		, bus_(fabric_, tgl)
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
		init_gpio();

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
		init_ipt_bus();

		//
		//	broker clients
		//
		CYNG_LOG_INFO(logger_, "initialize: broker clients");
		init_broker_clients();

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
		//cfg_lmn clmn(cfg_, lmn::WIRELESS);
		//std::cerr << clmn.get_port() << std::endl;
	}

	void bridge::init_lmn_ports() {

		init_lmn_port(lmn_type::WIRELESS);
		init_lmn_port(lmn_type::WIRED);
	}

	void bridge::init_lmn_port(lmn_type type) {

		cfg_lmn cfg(cfg_, type);
		auto const port = cfg.get_port();

		if (cfg.is_enabled()) {
			CYNG_LOG_INFO(logger_, "init LMN [" << port << "]");
			auto channel = ctl_.create_named_channel_with_ref<lmn>(port, ctl_, logger_, cfg_, type);
			BOOST_ASSERT(channel->is_open());
			channel->dispatch("open", cyng::make_tuple());
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN [" << port << "] is not enabled" );
		}
	}

	void bridge::init_ipt_bus() {
		//bus_
	}

	void bridge::init_broker_clients() {

		init_broker_clients(lmn_type::WIRELESS);
		init_broker_clients(lmn_type::WIRED);
	}

	void bridge::init_broker_clients(lmn_type type) {

		cfg_broker cfg(cfg_, type);
		auto const port = cfg.get_port();

		if (cfg.is_enabled()) {

			if (!cfg.is_lmn_enabled()) {
				CYNG_LOG_WARNING(logger_, "LMN for [" << port << "] is not running. This broker will never receive any data");
			}

			//	All broker for this port have the same name.
			auto const name = cfg.get_task_name();
			auto const timeout = cfg.get_timeout();
			auto const login = cfg.has_login();

			auto const size = cfg.size();
			CYNG_LOG_INFO(logger_, size << " broker \"" << name << "\" configured for [" << port << "]");
			auto const vec = cfg.get_broker_list();
			for (auto const& trg : vec) {

				//
				//	FixMe: add login required
				//
				auto channel = ctl_.create_named_channel_with_ref<broker>(name, ctl_, logger_, trg, timeout, login);
				BOOST_ASSERT(channel->is_open());
				channel->dispatch("start", cyng::make_tuple());

			}
		}
		else {
			CYNG_LOG_WARNING(logger_, "broker for [" << port << "] is not enabled");
		}
	}

	void bridge::init_gpio() {

		cfg_gpio cfg(cfg_);
		auto p = cfg.get_path();
		if (cfg.is_enabled()) {

			//
			//	get all pins and start a task for each pin
			//
			auto const pins = cfg.get_pins();
			for (auto const pin : pins) {
				auto const sp = cfg.get_path(pin);
				auto const name = cfg_gpio::get_name(pin);
				CYNG_LOG_INFO(logger_, "init GPIO [" << name << "]");
				auto channel = ctl_.create_named_channel_with_ref<gpio>(name, logger_, sp);
			}
		}
		else {
			CYNG_LOG_WARNING(logger_, "GPIO [" << p << "] is not enabled");
		}
	}

}