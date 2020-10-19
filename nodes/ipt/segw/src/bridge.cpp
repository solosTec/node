/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "bridge.h"
#include "cache.h"
#include "storage.h"
#include "segw.h"
#include "cfg_rs485.h"
#include "cfg_wmbus.h"

#include "tasks/gpio.h"
#include "tasks/obislog.h"
#include "tasks/clock.h"
#include "tasks/readout.h"
#include "tasks/limiter.h"

#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/event.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/util/split.h>
#include <cyng/table/meta.hpp>
#include <cyng/table/restore.h>
#include <cyng/buffer_cast.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/sys/port.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	bridge& bridge::get_instance(cyng::logging::log_ptr logger
		, cyng::async::mux& mux
		, cache& db
		, storage& store)
	{
		//
		//	no need to be threadsafe 
		//
		static bridge instance(logger, mux, db, store);
		return instance;
	}

	bridge::bridge(cyng::logging::log_ptr logger
		, cyng::async::mux& mux
		, cache& db
		, storage& store)
	: logger_(logger)
		, mux_(mux)
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
		load_data_collectors();
		load_data_mirror();
		load_iec_devices();
		load_privileges();
	}

	void bridge::finalize()
	{
		//
		//	register as listener
		//
		connect_to_cache();

		//
		//	log power return message
		//
		power_return();

		//
		//	store boot time
		//
		cache_.set_cfg("boot.time", std::chrono::system_clock::now());

		//
		//	reset IP-T master index (ROOT_IPT_PARAM:master)
		//
		cache_.set_cfg(build_cfg_key({ sml::OBIS_ROOT_IPT_PARAM }, "master"), static_cast<std::uint8_t>(0));

		//
		//	start task clock
		//
		start_task_clock();

		//
		//	start task OBISLOG (15 min)
		//
		start_task_obislog();

		//
		//	get maximum size of all data collectors and shrink tables
		//	if maximum size was exceeded.
		//
		start_task_limiter();

		//
		//	start GPIO task
		//
		start_task_gpio();

		//
		//	start readout task
		//
		start_task_readout();
	}

	void bridge::shrink()
	{
		//
		//	data collectors to remove
		//
		cyng::table::key_list_t to_remove;

		cache_.loop("_DataCollector", [&](cyng::table::record const& rec)->bool {

			auto const max_size = cyng::value_cast<std::uint16_t>(rec["maxSize"], 100);
			auto const srv_id = cyng::to_buffer(rec["serverID"]);
			auto const buffer = cyng::to_buffer(rec["profile"]);
			if (buffer.empty()) {

				CYNG_LOG_ERROR(logger_, "data collector #"
					<< cyng::numeric_cast(rec["nr"], 0)
					<< " of server "
					<< sml::from_server_id(srv_id)
					<< " has no profile");

				to_remove.push_back(rec.key());

			}
			else {
				sml::obis const profile(buffer);
				auto const count = storage_.shrink(max_size, srv_id, profile);
				CYNG_LOG_INFO(logger_, "profile "
					<< profile.to_str()
					<< " shrinked by "
					<< count
					<< " rows");
			}
			return true;	//	continue
		});

		//
		//	remove flagged data collectors
		//
		for (auto const& key : to_remove) {
			CYNG_LOG_WARNING(logger_, "delete data collector: "
				<< cyng::io::to_str(key));
			cache_.db_.erase("_DataCollector", key, cache_.get_tag());
		}
	}

	void bridge::load_configuration()
	{
		cache_.write_table("_Cfg", [&](cyng::store::table* tbl) {

			storage_.loop("TCfg", [&](cyng::table::record const& rec)->bool {

				auto const name = cyng::value_cast<std::string>(rec["path"], "");
				auto const key = cyng::table::key_generator(name);

				auto const type = cyng::value_cast(rec["type"], 15u);
				auto const val = cyng::value_cast<std::string>(rec["val"], "");

				try {

					//
					//	restore original data type from string
					//
					auto obj = cyng::table::restore(val, type);

					CYNG_LOG_TRACE(logger_, "load - "
						<< name
						<< " = "
						<< cyng::io::to_str(obj));

					//
					//	restore original data type from string and a type information
					//
					tbl->merge(key
						, cyng::table::data_generator(obj)
						, 1u	//	only needed for insert operations
						, cache_.get_tag());

					if (boost::algorithm::equals(name, sml::OBIS_SERVER_ID.to_str())) {

						//
						//	init server ID in cache
						//
						CYNG_LOG_INFO(logger_, "init server id with " << val);
						auto mac = cyng::value_cast(cyng::table::restore(val, type), cyng::generate_random_mac48());
						cache_.server_id_ = sml::to_gateway_srv_id(mac);
					}
				}
				catch (std::exception const& ex) {

					CYNG_LOG_ERROR(logger_, "cannot load "
						<< name
						<< ": "
						<< ex.what());
				}

				return true;	//	continue
			});
		});

		//
		//	check configuration
		//	"rs485.enabled" && "rs485.port"
		//	"IF_wMBUS:enabled" && "IF_wMBUS:port"
		//
		auto const ports = cyng::sys::get_ports();

		cfg_rs485 rs485(cache_);
		if (rs485.is_enabled()) {
			auto const port = rs485.get_port();
			if (std::none_of(ports.begin(), ports.end(), [port](std::string const& p) {
				return boost::algorithm::equals(port, p);
				})) {

				CYNG_LOG_ERROR(logger_, "rs485 port "
					<< port
					<< " is not available");

			}
			else {
				CYNG_LOG_TRACE(logger_, "rs485 port "
					<< port
					<< " is available");
			}
		}

		cfg_wmbus wmbus(cache_);
		if (wmbus.is_enabled()) {
			auto const port = wmbus.get_port();
			if (std::none_of(ports.begin(), ports.end(), [port](std::string const& p) {
				return boost::algorithm::equals(port, p);
				})) {

				CYNG_LOG_ERROR(logger_, "wireless M-Bus port "
					<< port
					<< " is not available");

			}
			else {
				CYNG_LOG_TRACE(logger_, "wireless M-Bus port "
					<< port
					<< " is available");
			}
		}
	}

	void bridge::load_devices_mbus()
	{
		cache_.write_table("_DeviceMBUS", [&](cyng::store::table * tbl) {
			storage_.loop("TDeviceMBUS", [&](cyng::table::record const& rec)->bool {

				if (tbl->insert(rec.key(), rec.data(), rec.get_generation(), cache_.get_tag())) {

					cyng::buffer_t const srv = cyng::to_buffer(rec.key().at(0));
					CYNG_LOG_TRACE(logger_, "load mbus device "
						<< sml::from_server_id(srv));
				}
				else {

					CYNG_LOG_ERROR(logger_, "loading TDeviceMBUS failed: "
						<< cyng::io::to_str(rec.key())
						<< ", body: "
						<< cyng::io::to_str(rec.data()));

				}

				return true;	//	continue
			});
		});
	}

	void bridge::load_data_collectors()
	{
		cache_.write_table("_DataCollector", [&](cyng::store::table* tbl) {
			storage_.loop("TDataCollector", [&](cyng::table::record const& rec)->bool {

				if (tbl->insert(rec.key(), rec.data(), rec.get_generation(), cache_.get_tag())) {

					cyng::buffer_t const srv = cyng::to_buffer(rec.key().at(0));
					CYNG_LOG_TRACE(logger_, "load data collector "
						<< sml::from_server_id(srv));
				}
				else {

					CYNG_LOG_ERROR(logger_, "loading TDataCollector failed: "
						<< cyng::io::to_str(rec.key())
						<< ", body: "
						<< cyng::io::to_str(rec.data()));

				}

				return true;	//	continue
				});
			});
	}

	void bridge::load_data_mirror()
	{
		cache_.write_table("_DataMirror", [&](cyng::store::table* tbl) {
			storage_.loop("TDataMirror", [&](cyng::table::record const& rec)->bool {

				if (tbl->insert(rec.key(), rec.data(), rec.get_generation(), cache_.get_tag())) {

					cyng::buffer_t const srv = cyng::to_buffer(rec.key().at(0));
					CYNG_LOG_TRACE(logger_, "load register "
						<< sml::from_server_id(srv));
				}
				else {

					CYNG_LOG_ERROR(logger_, "loading TDataMirror failed - key: "
						<< cyng::io::to_str(rec.key())
						<< ", body: "
						<< cyng::io::to_str(rec.data()));

				}

				return true;	//	continue
				});
			});
	}

	void bridge::load_iec_devices()
	{
		cache_.write_table("_IECDevs", [&](cyng::store::table* tbl) {
			storage_.loop("TIECDevs", [&](cyng::table::record const& rec)->bool {

				if (tbl->insert(rec.key(), rec.data(), rec.get_generation(), cache_.get_tag())) {

					cyng::buffer_t const srv = cyng::to_buffer(rec.data().at(0));
					CYNG_LOG_TRACE(logger_, "load IEC device "
						<< sml::from_server_id(srv));
				}
				else {

					CYNG_LOG_ERROR(logger_, "loading TIECDevs failed - key: "
						<< cyng::io::to_str(rec.key())
						<< ", body: "
						<< cyng::io::to_str(rec.data()));

				}

				return true;	//	continue
				});
			});
	}

	void bridge::load_privileges()
	{
		cache_.write_table("_User", [&](cyng::store::table* tbl) {
			storage_.loop("TUser", [&](cyng::table::record const& rec)->bool {

				//
				//	build a role bitmask 
				//
				std::uint8_t role{ 0 };
				if (cyng::value_cast(rec["rGuest"], false)) {
					role |= get_bit_position(sml::role::GUEST);
				}
				if (cyng::value_cast(rec["rUser"], false)) {
					role |= get_bit_position(sml::role::USER);
				}
				if (cyng::value_cast(rec["rGWOp"], false)) {
					role |= get_bit_position(sml::role::GW_OPERATOR);
				}
				if (cyng::value_cast(rec["rdevOp"], false)) {
					role |= get_bit_position(sml::role::DEV_OPERATOR);
				}
				if (cyng::value_cast(rec["rProv"], false)) {
					role |= get_bit_position(sml::role::SERVICE_PROVIDER);
				}
				if (cyng::value_cast(rec["rSupp"], false)) {
					role |= get_bit_position(sml::role::SUPPLIER);
				}
				if (cyng::value_cast(rec["rMan"], false)) {
					role |= get_bit_position(sml::role::MANUFACTURER);
				}
				if (cyng::value_cast(rec["rRes"], false)) {
					role |= get_bit_position(sml::role::ROLE_RESERVED);
				}

				if (tbl->insert(rec.key()
					, cyng::table::data_generator(role, rec["pwd"], rec["nr"])
					, rec.get_generation()
					, cache_.get_tag())) {

					CYNG_LOG_TRACE(logger_, "load user "
						<< cyng::io::to_str(rec["user"]));
				}
				else {

					CYNG_LOG_ERROR(logger_, "loading TUser failed: "
						<< cyng::io::to_str(rec.key())
						<< ", body: "
						<< cyng::io::to_str(rec.data()));
				}

				return true;	//	continue
				});
			});

		cache_.write_table("_Privileges", [&](cyng::store::table* tbl) {
			storage_.loop("TPrivileges", [&](cyng::table::record const& rec)->bool {

				if (tbl->insert(rec.key(), rec.data(), rec.get_generation(), cache_.get_tag())) {

					auto const srv = cyng::to_buffer(rec["meter"]);
					sml::obis const code(cyng::to_buffer(rec["reg"]));
					auto const priv = cyng::value_cast<std::uint8_t>(rec["priv"], 0);
					CYNG_LOG_TRACE(logger_, "load priv "
						<< sml::from_server_id(srv)
						<< " - "
						<< code.to_str()
						<< " - "
						<< +priv);
				}
				else {

					CYNG_LOG_ERROR(logger_, "loading TPrivileges failed - key: "
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
		auto const srv = cache_.get_srv_id();		

		storage_.generate_op_log(sw
			, sml::LOG_CODE_09	//	0x00100023 - power return
			, sml::OBIS_PEER_SCM	//	source is SCM
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
		if (boost::algorithm::equals(tbl->meta().get_name(), "_DeviceMBUS")) {

#ifdef _DEBUG
			//CYNG_LOG_TRACE(logger_, "Insert into table TDeviceMBUS key: "
			//	<< cyng::io::to_str(key)
			//	<< ", body: "
			//	<< cyng::io::to_str(body));
#endif

			if (!storage_.insert("TDeviceMBUS"
				, key
				, body
				, gen)) {

				CYNG_LOG_ERROR(logger_, "Insert into table TDeviceMBUS failed - key: " 
					<< cyng::io::to_str(key)
					<< ", body: "
					<< cyng::io::to_str(body));

			}
		}
#ifdef _DEBUG
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Readout")) {

			CYNG_LOG_TRACE(logger_, "readout complete - key: "
				<< cyng::io::to_str(key)
				<< ", body: "
				<< cyng::io::to_str(body));
		}
#endif
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_ReadoutData")) {

			CYNG_LOG_TRACE(logger_, "readout data - key: "
				<< cyng::io::to_str(key)
				<< ", body: "
				<< cyng::io::to_str(body));
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DataCollector")) {

			if (!storage_.insert("TDataCollector"
				, key
				, body
				, gen)) {

				CYNG_LOG_ERROR(logger_, "Insert into table TDataCollector failed - key: "
					<< cyng::io::to_str(key)
					<< ", body: "
					<< cyng::io::to_str(body));

			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_PushOps")) {

			//
			//	<network> has started a <push> task
			//
			cyng::table::record rec(tbl->meta_ptr(), key, body, gen);

			if (!storage_.insert("TPushOps"
				, key
				, rec.shrink_data({ "tsk" })	//	remove "tsk" cell
				, gen)) {

				CYNG_LOG_ERROR(logger_, "Insert into table TPushOps failed - key: "
					<< cyng::io::to_str(key)
					<< ", body: "
					<< cyng::io::to_str(body));

			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DataMirror")) {

			if (!storage_.insert("TDataMirror"
				, key
				, body
				, gen)) {

				CYNG_LOG_ERROR(logger_, "Insert into table TDataMirror failed - key: "
					<< cyng::io::to_str(key)
					<< ", body: "
					<< cyng::io::to_str(body));

			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_IECDevs")) {

			if (!storage_.insert("TIECDevs"
				, key
				, body
				, gen)) {

				CYNG_LOG_ERROR(logger_, "Insert into table TIECDevs failed - key: "
					<< cyng::io::to_str(key)
					<< ", body: "
					<< cyng::io::to_str(body));

			}
		}
	}

	void bridge::sig_ins_cfg(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& body
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));
		BOOST_ASSERT(tbl->meta().check_key(key));
		BOOST_ASSERT(tbl->meta().check_body(body));
		BOOST_ASSERT(key.size() == 1);
		BOOST_ASSERT(body.size() == 1);

		//
		//	store values as string
		//
		auto val = cyng::io::to_str(body.at(0));
		storage_.insert("TCfg", key, cyng::table::data_generator(val, val, body.at(0).get_class().tag()), gen);
	}


	void bridge::sig_del(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, boost::uuids::uuid source)
	{
		//
		//	delete from SQL database
		//
		if (boost::algorithm::equals(tbl->meta().get_name(), "_DataCollector")) {

			if (!storage_.remove("TDataCollector", key)) {

				CYNG_LOG_ERROR(logger_, "delete from TDataCollector failed: "
					<< cyng::io::to_str(key));

			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_PushOps")) {

			if (!storage_.remove("TPushOps", key)) {

				CYNG_LOG_ERROR(logger_, "delete from TPushOps failed: "
					<< cyng::io::to_str(key));

			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DataMirror")) {

			if (!storage_.remove("TDataMirror", key)) {

				CYNG_LOG_ERROR(logger_, "delete from TDataMirror failed: "
					<< cyng::io::to_str(key));
			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_IECDevs")) {

			if (!storage_.remove("TIECDevs", key)) {

				CYNG_LOG_ERROR(logger_, "delete from TIECDevs failed: "
					<< cyng::io::to_str(key));

			}
		}
	}

	void bridge::sig_del_cfg(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, boost::uuids::uuid source)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));
		if (!storage_.remove("TCfg", key)) {

			CYNG_LOG_ERROR(logger_, "delete from TCfg failed: "
				<< cyng::io::to_str(key));
		}
	}

	void bridge::sig_clr(cyng::store::table const*, boost::uuids::uuid)
	{
		//
		//	ToDo: clear SQL table
		//


	}

	void bridge::sig_clr_cfg(cyng::store::table const* tbl, boost::uuids::uuid)
	{
		//
		//	ToDo: clear SQL table
		//
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));


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

		if (boost::algorithm::equals(tbl->meta().get_name(), "_DeviceMBUS")) {

#ifdef _DEBUG
			CYNG_LOG_TRACE(logger_, "Update TDeviceMBUS key: "
				<< cyng::io::to_str(key)
				<< ", "
				<< attr.first
				<< ": "
				<< cyng::io::to_str(attr.second));
#endif

			//
			//	Update "TDeviceMBUS"
			//
			storage_.update("TDeviceMBUS"
				, key
				, tbl->meta().to_param(attr)
				, gen);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DataCollector")) {

			storage_.update("TDataCollector"
				, key
				, tbl->meta().to_param(attr)
				, gen);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_PushOps")) {

			auto const name = tbl->meta().get_body_name(attr.first);

			//
			//	skip "tsk" attribute
			//
			if (!boost::algorithm::equals(name, "tsk")) {
				if (storage_.update("TPushOps"
					, key
					, tbl->meta().to_param(attr)
					, gen)) {

					//
					//	update task
					//
					auto const rec = tbl->lookup(key);
					auto const tsk = cyng::value_cast<std::size_t>(rec["tsk"], cyng::async::NO_TASK);
					if (tsk != cyng::async::NO_TASK) {
						if (boost::algorithm::equals(name, "target")) {
							mux_.post(tsk, name, cyng::tuple_t({ attr.second }));
						}
						else if (boost::algorithm::equals(name, "interval") || boost::algorithm::equals(name, "delay")) {
							mux_.post(tsk, name, cyng::tuple_factory(name, attr.second));

						}
					}
				}
				else {
					CYNG_LOG_ERROR(logger_, "to update table _PushOps failed: "
						<< name
						<< " = "
						<< cyng::io::to_str(attr.second));

				}
			}
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_DataMirror")) {

			storage_.update("TDataMirror"
				, key
				, tbl->meta().to_param(attr)
				, gen);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_IECDevs")) {

			storage_.update("TIECDevs"
				, key
				, tbl->meta().to_param(attr)
				, gen);
		}
	}

	void bridge::sig_mod_cfg(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid tag)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));

		auto const name = cyng::value_cast<std::string>(key.at(0), "");
		if (boost::algorithm::equals(name, "status.word")) {

			//LOG_CODE_24 = 0x02100008,	//	Ethernet - Link an KundenSchnittstelle aktiviert
			//EVT_SOURCE_39 = 0x818100000011,	//	USERIF(Interne MUC - SW - Funktion)

			//
			//	write into "op.log"
			//
			std::uint64_t sw = cyng::value_cast(attr.second, sml::status::get_initial_value());
			storage_.generate_op_log(sw
				, 0x800008	//	evt
				, sml::OBIS_PEER_USERIF	//	source
				, cyng::make_buffer({ 0x81, 0x81, 0x00, 0x00, 0x00, 0x11 })	//	server ID
				, ""	//	target
				, 0		//	nr
				, "");	//	description
		}
		else if (boost::algorithm::equals(name, sml::OBIS_SERVER_ID.to_str())) {

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

	void bridge::sig_trx(cyng::store::trx_type trx)
	{
		switch(trx) {
		case cyng::store::trx_type::START:
			CYNG_LOG_TRACE(logger_, "trx begin");
			storage_.trx_start();
			break;
		case cyng::store::trx_type::COMMIT:
			CYNG_LOG_TRACE(logger_, "trx commit");
			storage_.trx_commit();
			break;
			//case cyng::store::trx_type::ROLLBACK:
		default:
			CYNG_LOG_TRACE(logger_, "trx rollback");
			storage_.trx_rollback();
			break;
		}
	}

	void bridge::connect_to_cache()
	{
		auto l = cache_.db_.get_listener("_Cfg"
			, std::bind(&bridge::sig_ins_cfg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del_cfg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr_cfg, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod_cfg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
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

		l = cache_.db_.get_listener("_DataCollector"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		l = cache_.db_.get_listener("_PushOps"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		l = cache_.db_.get_listener("_DataMirror"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		l = cache_.db_.get_listener("_IECDevs"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		cache_.db_.get_trx_listener(std::bind(&bridge::sig_trx, this, std::placeholders::_1));

	}

	void bridge::start_task_clock()
	{
		auto const tid = cyng::async::start_task_detached<clock>(mux_
			, logger_
			, this->cache_
			, std::chrono::minutes(2));
		boost::ignore_unused(tid);
	}

	void bridge::start_task_obislog()
	{
		auto const interval = cache_.get_cfg(sml::OBIS_OBISLOG_INTERVAL.to_str(), 15);
		
		auto const tid = cyng::async::start_task_detached<obislog>(mux_
			, logger_
			, *this
			, std::chrono::minutes(interval));
		boost::ignore_unused(tid);
	}

	void bridge::start_task_limiter()
	{
		auto const tid = cyng::async::start_task_detached<limiter>(mux_
			, logger_
			, *this
			, std::chrono::hours(1));
		boost::ignore_unused(tid);
	}

	void bridge::start_task_gpio()
	{
		//
		//	start one task for every GPIO
		//
		//gpio-path|1|/sys/class/gpio|/sys/class/gpio|15
		//gpio-vector|1|46 47 50 53|46 47 50 53|15

		auto const gpio_enabled = cache_.get_cfg("gpio-enabled", 
#if defined(__ARMEL__)
			true
#else
			false
#endif
		);
		auto const gpio_path = cache_.get_cfg<std::string>("gpio-path", "/sys/class/gpio");
		auto const gpio_vector = cache_.get_cfg<std::string>("gpio-vector", "46 47 50 53");

		if (gpio_enabled) {
			//
			//	Start a GPIO task for every GPIO
			//
			auto const svec = cyng::split(gpio_vector, " ");
			for (auto const& s : svec) {
				auto const p = cyng::filesystem::make_path(gpio_path) / ("gpio" + s);
				auto const tid = cyng::async::start_task_detached<gpio>(mux_
					, logger_
					, p);

				//
				//	store task id in cache DB
				//
				cache_.set_cfg("gpio-task-" + s, tid);
			}
		}
		else {
			CYNG_LOG_WARNING(logger_, "GPIO control disabled");
		}
	}

	void bridge::start_task_readout()
	{
		auto const interval = cache_.get_cfg("readout-interval", 122);
		auto const tid = cyng::async::start_task_detached<readout>(mux_
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
