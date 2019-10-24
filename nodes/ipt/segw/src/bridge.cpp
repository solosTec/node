/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "bridge.h"
#include "cache.h"
#include "storage.h"
#include "tasks/gpio.h"
#include "tasks/obislog.h"

#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/event.h>

#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/util/split.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	bridge::bridge(cyng::async::mux& mux, cyng::logging::log_ptr logger, cache& db, storage& store)
		: logger_(logger)
		, cache_(db)
		, storage_(store)
	{
		//
		//	Preload cache with configuration data.
		//	Preload must be done before we connect to the cache. Otherwise
		//	we would receive our own changes.
		//
		load_configuration();

		//
		//	register as listener
		//
		connect_to_cache();

		//
		//	start task OBISLOG (15 min)
		//
		start_task_obislog(mux);

		//
		//	start task GPIO task
		//
		start_task_gpio(mux);

	}

	void bridge::load_configuration()
	{
		cache_.db_.access([&](cyng::store::table* tbl) {

			storage_.loop("TCfg", [&](cyng::table::record const& rec)->bool {

				auto const name = cyng::value_cast<std::string>(rec["path"], "");
				auto const type = cyng::value_cast(rec["type"], 15);
				auto const val = cyng::value_cast<std::string>(rec["val"], "");

				switch (type) {
				case cyng::TC_BOOL:
					//	true is encoded as "1"
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(boost::algorithm::equals("1", val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				//case cyng::TC_CHAR:
				case cyng::TC_FLOAT:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stof(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				case cyng::TC_DOUBLE:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stod(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				case cyng::TC_FLOAT80:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stold(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				//case cyng::TC_UINT8:
				//case cyng::TC_UINT16:
				case cyng::TC_UINT32:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stoul(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				case cyng::TC_UINT64:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stoull(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				//case cyng::TC_INT8:
				//case cyng::TC_INT16: 
				case cyng::TC_INT32:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stoi(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				case cyng::TC_INT64:
					tbl->update(cyng::table::key_generator(name)
						, cyng::table::data_generator(std::stoll(val))
						, 1u	//	only needed for insert operations
						, cache_.get_tag());
					break;
				//case cyng::TC_STRING:
				//case cyng::TC_TIME_POINT: 
				//case cyng::TC_NANO_SECOND:
				//case cyng::TC_MICRO_SECOND: 
				//case cyng::TC_MILLI_SECOND:
				//case cyng::TC_SECOND:
				//case cyng::TC_MINUTE:
				//case cyng::TC_HOUR:

				//case cyng::TC_DBL_TP:
				//case cyng::TC_VERSION:
				//case cyng::TC_REVISION:
				//case cyng::TC_CODE:
				//case cyng::TC_LABEL:
				//case cyng::TC_SEVERITY:
				//case cyng::TC_BUFFER:
				//case cyng::TC_MAC48:
				//case cyng::TC_MAC64:
				//case cyng::TC_COLOR_8:
				//case cyng::TC_COLOR_16:
					break;

				default:
				tbl->update(cyng::table::key_generator(name)
					, cyng::table::data_generator(rec["val"])
					, 1u	//	only needed for insert operations
					, cache_.get_tag());
				break;
				}
				return true;	//	continue
			});

			//
			//	store boot time
			//
			cache_.set_config_value(tbl, "boot.time", std::chrono::system_clock::now());

		}, cyng::store::write_access("_Cfg"));
	}

	void bridge::power_return()
	{
		auto const sw = cache_.get_status_word();

		storage_.generate_op_log(sw
			, sml::LOG_CODE_09	//	0x00100023 - power return
			, sml::OBIS_CODE_PEER_SCM	//	source is SCM
			, cyng::make_buffer({ })	//	server ID
			, ""	//	target
			, 0		//	nr
			, "");	//	description
	}

	void bridge::sig_ins(cyng::store::table const*
		, cyng::table::key_type const&
		, cyng::table::data_type const&
		, std::uint64_t
		, boost::uuids::uuid)
	{

	}

	void bridge::sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid)
	{

	}

	void bridge::sig_clr(cyng::store::table const*, boost::uuids::uuid)
	{

	}

	void bridge::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid tag)
	{
		//
		//	Get the name of the column to be modified.
		//
		auto const name = tbl->meta().get_name(attr.first);

		if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
			
			if (boost::algorithm::equals(name, "status.word")) {

				//LOG_CODE_24 = 0x02100008,	//	Ethernet - Link an KundenSchnittstelle aktiviert
				//EVT_SOURCE_39 = 0x818100000011,	//	USERIF(Interne MUC - SW - Funktion)

				//
				//	write into "op.log"
				//
				std::uint64_t sw = cyng::value_cast(attr.second, sml::status::get_initial_value());
				storage_.generate_op_log(sw
					, 0x800008	//	evt
					, sml::OBIS_CODE_PEER_USERIF	//	source
					, cyng::make_buffer({ 0x81, 0x81, 0x00, 0x00, 0x00, 0x11 })	//	server ID
					, ""	//	target
					, 0		//	nr
					, "");	//	description
			}
			else {

				//
				//	Update "TCfg"
				//
				storage_.update("TCfg", name, attr.second);
			}
		}
	}


	void bridge::connect_to_cache()
	{
		cache_.db_.get_listener("_Cfg"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	}

	void bridge::start_task_obislog(cyng::async::mux& mux)
	{
		auto const cycle_time = cache_.get_cfg("obis-log", 15);
		cyng::async::start_task_detached<obislog>(mux
			, logger_
			, std::chrono::minutes(cycle_time));

	}

	void bridge::start_task_gpio(cyng::async::mux& mux)
	{
		//
		//	start one task for every GPIO
		//
		//gpio-path|1|/sys/class/gpio|/sys/class/gpio|15
		//gpio-vector|1|46 47 50 53|46 47 50 53|15

		auto const gpio_path = cache_.get_cfg<std::string>("gpio-path", "/sys/class/gpio");
		auto const gpio_vector = cache_.get_cfg<std::string>("gpio-vector", "46 47 50 53");

		//
		//	Start a GPIO task for every GPIO
		//
		auto const svec = cyng::split(gpio_vector, " ");
		for (auto const& s : svec) {
			auto tid = cyng::async::start_task_detached<gpio>(mux
				, logger_
				, boost::filesystem::path(gpio_path) / ("/gpio" + s));

			//
			//	ToDo: store task id in cache DB
			//
		}
	}


}
