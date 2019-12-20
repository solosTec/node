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
#include "tasks/readout.h"

#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/event.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/util/split.h>
#include <cyng/table/meta.hpp>
#include <cyng/table/restore.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	bridge& bridge::get_instance(cyng::async::mux& mux, cyng::logging::log_ptr logger, cache& db, storage& store)
	{
		//
		//	no need to be threadsafe 
		//
		static bridge instance(mux, logger, db, store);
		return instance;
	}

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
		load_devices_mbus();

		//
		//	register as listener
		//
		connect_to_cache();

		//
		//	store boot time
		//
		//cache_.set_config_value(tbl, "boot.time", std::chrono::system_clock::now());
		cache_.set_cfg("boot.time", std::chrono::system_clock::now());

		//
		//	start task OBISLOG (15 min)
		//
		start_task_obislog(mux);

		//
		//	start GPIO task
		//
		start_task_gpio(mux);

		//
		//	start readout task
		//
		start_task_readout(mux);
	}

	void bridge::load_configuration()
	{
		cache_.write_table("_Cfg", [&](cyng::store::table* tbl) {

			storage_.loop("TCfg", [&](cyng::table::record const& rec)->bool {

				auto const name = cyng::value_cast<std::string>(rec["path"], "");
				auto const key = cyng::table::key_generator(name);

				auto const type = cyng::value_cast(rec["type"], 15u);
				auto const val = cyng::value_cast<std::string>(rec["val"], "");

				//
				//	restore original data type from string and a type information
				//
				tbl->merge(key
					, cyng::table::data_generator(cyng::table::restore(val, type))
					, 1u	//	only needed for insert operations
					, cache_.get_tag());

				if (boost::algorithm::equals(name, sml::OBIS_CODE_SERVER_ID.to_str())) {

					//
					//	init server ID in cache
					//
					CYNG_LOG_INFO(logger_, "init server id with " << val);
					auto mac = cyng::value_cast(cyng::table::restore(val, type), cyng::generate_random_mac48());
					cache_.server_id_ = sml::to_gateway_srv_id(mac);
				}

				return true;	//	continue
			});

		});
	}

	void bridge::load_devices_mbus()
	{
		cache_.write_table("_DeviceMBUS", [&](cyng::store::table * tbl) {
			storage_.loop("TDeviceMBUS", [&](cyng::table::record const& rec)->bool {

				if (tbl->insert(rec.key(), rec.data(), rec.get_generation(), cache_.get_tag())) {

					cyng::buffer_t srv;
					srv = cyng::value_cast(rec.key().at(0), srv);
					CYNG_LOG_TRACE(logger_, "load mbus device "
						<< sml::from_server_id(srv));
				}
				else {

					CYNG_LOG_ERROR(logger_, "insert into table TDeviceMBUS failed - key: "
						<< cyng::io::to_str(rec.key())
						<< ", body: "
						<< cyng::io::to_str(rec.data()));

				}

				return true;	//	continue
			});
		});
	}

	void bridge::power_return()
	{
		auto const sw = cache_.get_status_word();
		auto srv = cache_.get_srv_id();		

		storage_.generate_op_log(sw
			, sml::LOG_CODE_09	//	0x00100023 - power return
			, sml::OBIS_CODE_PEER_SCM	//	source is SCM
			, srv	//	server ID
			, ""	//	target
			, 0		//	nr
			, "power return");	//	description
	}

	void bridge::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& body
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		BOOST_ASSERT(tbl->meta().check_key(key));
		BOOST_ASSERT(tbl->meta().check_body(body));

		//
		//	insert into SQL database
		//
		if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {

			BOOST_ASSERT(key.size() == 1);
			BOOST_ASSERT(body.size() == 1);

			//
			//	store values as string
			//
			auto val = cyng::io::to_str(body.at(0));
			storage_.insert("TCfg", key, cyng::table::data_generator(val, val, body.at(0).get_class().tag()), gen, source);

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DeviceMBUS")) {

#ifdef _DEBUG
			//CYNG_LOG_TRACE(logger_, "Insert into table TDeviceMBUS key: "
			//	<< cyng::io::to_str(key)
			//	<< ", body: "
			//	<< cyng::io::to_str(body));
#endif

			if (!storage_.insert("TDeviceMBUS"
				, key
				, body
				, gen
				, source)) {

				CYNG_LOG_ERROR(logger_, "Insert into table TDeviceMBUS failed - key: " 
					<< cyng::io::to_str(key)
					<< ", body: "
					<< cyng::io::to_str(body));

			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Readout")) {

			CYNG_LOG_TRACE(logger_, "readout complete - key: "
				<< cyng::io::to_str(key)
				<< ", body: "
				<< cyng::io::to_str(body));
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_ReadoutData")) {

			CYNG_LOG_TRACE(logger_, "readout data - key: "
				<< cyng::io::to_str(key)
				<< ", body: "
				<< cyng::io::to_str(body));
		}
		
	}

	void bridge::sig_del(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, boost::uuids::uuid source)
	{
		//
		//	delete from SQL database
		//
		if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {

			storage_.remove("TCfg", key, source);
		}
	}

	void bridge::sig_clr(cyng::store::table const*, boost::uuids::uuid)
	{
		//
		//	ToDo: clear SQL table
		//


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
			else if (boost::algorithm::equals(name, sml::OBIS_CODE_SERVER_ID.to_str())) {

				//
				//	update server ID in cache
				//
				auto mac = cyng::value_cast(attr.second, cyng::generate_random_mac48());
				cache_.server_id_ = sml::to_gateway_srv_id(mac);
			}


			//
			//	Update "TCfg"
			//
			storage_.update("TCfg"
				, key
				, "val"
				, attr.second
				, gen);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DeviceMBUS")) {

			//
			//	Update "TDeviceMBUS"
			//
			storage_.update("TDeviceMBUS"
				, key
				, tbl->meta().to_param(attr)
				, gen);
		}
	}

	void bridge::connect_to_cache()
	{
		auto l = cache_.db_.get_listener("_Cfg"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		boost::ignore_unused(l);

		l = cache_.db_.get_listener("_DeviceMBUS"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

#ifdef _DEBUG
		l = cache_.db_.get_listener("_Readout"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		l = cache_.db_.get_listener("_ReadoutData"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
#endif
	}

	void bridge::start_task_obislog(cyng::async::mux& mux)
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
			auto const tid = cyng::async::start_task_detached<gpio>(mux
				, logger_
				, boost::filesystem::path(gpio_path) / ("/gpio" + s));

			//
			//	store task id in cache DB
			//
			cache_.set_cfg("gpio-task-" + s, tid);
		}
	}

	void bridge::start_task_gpio(cyng::async::mux& mux)
	{
		auto const interval = cache_.get_cfg("readout-interval", 122);
		auto const tid = cyng::async::start_task_detached<readout>(mux
			, logger_
			, cache_
			, storage_
			, std::chrono::seconds(interval));

	}

	void bridge::start_task_readout(cyng::async::mux& mux)
	{
		auto const interval = cache_.get_cfg("readout-interval", 122);
		auto const tid = cyng::async::start_task_detached<readout>(mux
			, logger_
			, cache_
			, storage_
			, std::chrono::seconds(interval));
	}

	void bridge::generate_op_log(sml::obis peer
		, std::uint32_t evt
		, std::string target
		, std::uint8_t nr
		, std::string details)
	{
		auto const sw = cache_.get_status_word();
		auto srv = cache_.get_srv_id();

		storage_.generate_op_log(sw
			, evt
			, peer	//	source
			, srv	//	server ID
			, target
			, nr	
			, details);	
	}

}
