/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "db.h"
#include "../../shared/db/db_schemes.h"
#include <NODE_project_info.h>
#include <cyng/table/meta.hpp>
#include <cyng/intrinsics/traits/tag.hpp>
#include <cyng/intrinsics/traits.hpp>
#include <cyng/io/serializer.h>

#include <algorithm>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/process/environment.hpp>
#include <boost/algorithm/string.hpp>


namespace node 
{
	void init(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::uuids::uuid tag
		, std::string country_code
		, boost::asio::ip::tcp::endpoint ep
		, std::uint64_t global_config
		, boost::filesystem::path stat_dir
		, std::uint64_t max_messages)
	{
		CYNG_LOG_INFO(logger, "initialize database as node " << tag);

		//
		//	TDevice table
		//
		//	(1) tag (UUID) - pk
		//	+-----------------
		//	(2) name [std::string]
		//	(3) number [std::string]
		//	(4) [std::string] description 
		//	(5) [std::string] identifier (device identifier/type)
		//	(6) [std::string] firmware version 
		//	(7) [bool] enabled (only login allowed)
		//	(8) [std::chrono::system_clock::time_stamp] created 
		//	(9) [std::uint32] query

		if (!create_table(db, "TDevice"))	{
			CYNG_LOG_FATAL(logger, "cannot create table TDevice");
		}

		//	(1) tag (UUID) - pk
		//	+-----------------
		//	(1) id (server/gateway ID) - unique
		//	(2) manufacturer (i.e. EMH)
		//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
		//	(4) production date/time
		//	(5) firmwareversion (i.e. 11600000)
		//	(6) fabrik nummer (i.e. 06441734)
		//	(7) MAC of service interface
		//	(8) MAC od data interface
		//	(9) Default PW
		//	(10) root PW
		//	(11) W-Mbus ID (i.e. A815408943050131)
		//	(12) source (UUID) - usefull to detect multiple configuration uploads
		if (!create_table(db, "TGateway")) {
			CYNG_LOG_FATAL(logger, "cannot create table TGateway");
		}
		else
		{
#ifdef _DEBUG
			db.insert("TGateway"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator("0500153B02517E"
					, "EMH"
					, std::chrono::system_clock::now()
					, "06441734"
					, cyng::mac48(0, 1, 2, 3, 4, 5)
					, cyng::mac48(0, 1, 2, 3, 4, 6)
					, "secret"
					, "secret"
					, "mbus"
					, "operator"
					, "operator")
				, 94
				, tag);
#endif
		}

		//	(1) tag (UUID) - pk
		//	+-----------------
		//	(1) id (server/gateway ID) - unique
		//	(2) AESKey (128 bit encryption)
		//	(3) driver
		//	(4) activation
		//	(5) DevAddr
		//	(6) AppEUI - provided by the owner of the application server
		//	(7) GatewayEUI
		if (!create_table(db, "TLoRaDevice"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TLoRaDevice");
		}
#ifdef _DEBUG
		else
		{
			db.insert("TLoRaDevice"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator(cyng::mac64(0, 1, 2, 3, 4, 5, 6, 7)
					, "1122334455667788990011223344556677889900112233445566778899001122"
					, "demo"	//	driver
					, true	//	OTAA
					, 0x64	//	DevAddr
					, cyng::mac64(0, 1, 2, 3, 4, 6, 7, 8)
					, cyng::mac64(0, 1, 2, 3, 4, 6, 7, 8))
				, 8
				, tag);
		}
#endif


		if (!create_table(db, "TMeter"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TMeter");
		}
#ifdef _DEBUG
		else
		{
			db.insert("TMeter"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator("01-e61e-13090016-3c-07"
					, "16000913"
					, "CH9876501234500A7T839KH38O2D78R45"
					, "ACME"	//	manufacturer
					, std::chrono::system_clock::now()
					, "11600000"
					, "16A098828.pse"
					, "06441734"
					, "NXT4-S20EW-6N00-4000-5020-E50/Q"
					, "Q3"
					, boost::uuids::string_generator()("8d04b8e0-0faf-44ea-b32b-8405d407f2c1"))	//	reference to TGateway 8d04b8e0-0faf-44ea-b32b-8405d407f2c1
				, 5
				, tag);
		}
#endif
		//
		//	TLL table - leased lines
		//
		//	(1) first (UUID) - pk
		//	(2) second (UUID) - pk
		//	+-----------------
		//	(3) description [std::string]
		//	(4) creation-time [std::chrono::system_clock::time_stamp]
		if (!create_table(db, "TLL"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TLL");
		}
		else
		{
			//db.insert("TLL"
			//	, cyng::table::key_generator(tag)
			//	, cyng::table::data_generator("name", "number", "descr", "id", "vFirmware", true, std::chrono::system_clock::now())
			//	, 72
			//	, tag);

		}

		//
		//	The session tables uses the same tag as the remote client session
		//	
		if (!create_table(db, "_Session"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Session");
		}

		if (!create_table(db, "_Target"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Target");
		}

		//
		//	ack-time: the time interval (in seconds) in which a Push Data Transfer Response is expected
		//	after the transmission of the last character of a Push Data Transfer Request.
		//
		if (!db.create_table(cyng::table::make_meta_table<3, 8>("_Channel", 
			{ "channel"		//	[uint32] primary key 
			, "source"		//	[uint32] primary key 
			, "target"		//	[uint32] primary key 
			, "tag"			//	[uuid] remote target session tag
			, "peerTarget"	//	[object] target peer session
			, "peerChannel"	//	[uuid] owner/channel peer session
			, "pSize"		//	[uint16] - max packet size
			, "ackTime"		//	[uint32] - See description above
			, "count"		//	[size_t] target count
			, "owner"		//	[string] owner name of the channel
			, "name"		//	[string] name of push target
			},
			{	
				cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32, 
				cyng::TC_UUID, cyng::traits::PREDEF_SESSION, cyng::TC_UUID, cyng::TC_UINT16, cyng::TC_UINT32, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_STRING
			},
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 64 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Channel");
		}

		//
		//	All dial-up connections. Leased Lines have to be incorporated.
		//
		if (!create_table(db, "_Connection"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Connection");
		}

		if (!create_table(db, "_Cluster"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Cluster");
		}
		else
		{
			db.insert("_Cluster"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator("master"
					, std::chrono::system_clock::now()
					, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
					, 0u	//	no clients yet
					, std::chrono::microseconds(0)
					, ep
					, boost::this_process::get_id()
					, cyng::null())	//	no session instance
				, 1
				, tag);

		}

		if (!create_table(db, "_Config"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Config");
		}
		else
		{
			bool const connection_auto_login = is_connection_auto_login(global_config);
			bool const connection_auto_enabled = is_connection_auto_enabled(global_config);
			bool const connection_superseed = is_connection_superseed(global_config);
			bool const generate_time_series = is_generate_time_series(global_config);
			bool const catch_meters = is_catch_meters(global_config);
			bool const catch_lora = is_catch_lora(global_config);

			db.insert("_Config", cyng::table::key_generator("startup"), cyng::table::data_generator(std::chrono::system_clock::now()), 1, tag);
			db.insert("_Config", cyng::table::key_generator("master-tag"), cyng::table::data_generator(tag), 1, tag);
			db.insert("_Config", cyng::table::key_generator("connection-auto-login"), cyng::table::data_generator(connection_auto_login), 1, tag);
			db.insert("_Config", cyng::table::key_generator("connection-auto-enabled"), cyng::table::data_generator(connection_auto_enabled), 1, tag);
			db.insert("_Config", cyng::table::key_generator("connection-superseed"), cyng::table::data_generator(connection_superseed), 1, tag);
			db.insert("_Config", cyng::table::key_generator("generate-time-series"), cyng::table::data_generator(generate_time_series), 1, tag);
			db.insert("_Config", cyng::table::key_generator("catch-meters"), cyng::table::data_generator(catch_meters), 1, tag);
			db.insert("_Config", cyng::table::key_generator("catch-lora"), cyng::table::data_generator(catch_lora), 1, tag);

			db.insert("_Config", cyng::table::key_generator("connection-auto-login-default"), cyng::table::data_generator(connection_auto_login), 1, tag);
			db.insert("_Config", cyng::table::key_generator("connection-auto-enabled-default"), cyng::table::data_generator(connection_auto_enabled), 1, tag);
			db.insert("_Config", cyng::table::key_generator("connection-superseed-default"), cyng::table::data_generator(connection_superseed), 1, tag);
			db.insert("_Config", cyng::table::key_generator("generate-time-series-default"), cyng::table::data_generator(generate_time_series), 1, tag);
			db.insert("_Config", cyng::table::key_generator("catch-meters-default"), cyng::table::data_generator(catch_meters), 1, tag);
			db.insert("_Config", cyng::table::key_generator("catch-lora-default"), cyng::table::data_generator(catch_lora), 1, tag);

			db.insert("_Config", cyng::table::key_generator("generate-time-series-dir"), cyng::table::data_generator(stat_dir.string()), 1, tag);
			db.insert("_Config", cyng::table::key_generator("country-code"), cyng::table::data_generator(country_code), 1, tag);
			db.insert("_Config", cyng::table::key_generator("max-messages"), cyng::table::data_generator(max_messages), 1, tag);


			//	get hostname
			boost::system::error_code ec;
			const auto host_name = boost::asio::ip::host_name(ec);
			if (!ec) {
				db.insert("_Config", cyng::table::key_generator("host-name"), cyng::table::data_generator(host_name), 1, tag);
			}
			else {
				db.insert("_Config", cyng::table::key_generator("host-name"), cyng::table::data_generator(ec.message()), 1, tag);
			}

			db.insert("_Config", cyng::table::key_generator("smf-version"), cyng::table::data_generator(NODE_SUFFIX), 1, tag);

		}

		if (!create_table(db, "_SysMsg"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _SysMsg");
		}
		else
		{
			std::stringstream ss;
			boost::system::error_code ec;
			auto host = boost::asio::ip::host_name(ec);
			if (!ec)
			{ 
				ss
					<< "startup master node "
					<< tag
					<< " on "
					<< host
					;
			}
			else
			{
				ss
					<< "startup master node "
					<< tag
					;
			}
			insert_msg(db, cyng::logging::severity::LEVEL_INFO, ss.str(), tag);
		}

		//
		//	CSV task
		//
		if (!create_table(db, "_CSV"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _CSV");
		}

		//
		//	LoRa Uplink
		//
		if (!create_table(db, "_LoRaUplink"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _LoRaUplink");
		}

		//
		//	time series
		//
		if (!create_table(db, "_TimeSeries"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _TimeSeries");
		}

	}

	cyng::object get_config(cyng::store::db& db, std::string key)
	{
		cyng::object obj;
		db.access([&obj, key](cyng::store::table const* tbl_cfg)->void {
			obj = get_config(tbl_cfg, key);
		}, cyng::store::read_access("_Config"));
		return obj;
	}

	cyng::object get_config(cyng::store::table const* tbl, std::string key)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "_Config")) {
			auto const rec = tbl->lookup(cyng::table::key_generator(key));
			if (!rec.empty())	return rec["value"];
		}
		return cyng::make_object();
	}

	void insert_msg(cyng::store::db& db
		, cyng::logging::severity level
		, std::string const& msg
		, boost::uuids::uuid tag)
	{
		db.access([&](cyng::store::table* tbl, cyng::store::table const* tbl_cfg)->void {
			auto rec = tbl_cfg->lookup(cyng::table::key_generator("max-messages"));
			const std::uint64_t max_messages = (!rec.empty())
				? cyng::value_cast<std::uint64_t>(rec["value"], 1000u)
				: 1000u
				;

			insert_msg(tbl, level, msg, tag, max_messages);
		}	, cyng::store::write_access("_SysMsg")
			, cyng::store::read_access("_Config"));

	}

	void insert_msg(cyng::store::table* tbl
		, cyng::logging::severity level
		, std::string const& msg
		, boost::uuids::uuid tag
		, std::uint64_t max_messages)
	{
		//
		//	upper limit is 1000 messages
		//
		if (tbl->size() > max_messages)
		{
			auto max_rec = tbl->max_record();
			if (!max_rec.empty()) {

				//	get next message id
				auto next_idx = cyng::value_cast<std::uint64_t>(max_rec["id"], 0u);

				tbl->insert(cyng::table::key_generator(++next_idx)
					, cyng::table::data_generator(std::chrono::system_clock::now()
						, static_cast<std::uint8_t>(level), msg)
					, 1, tag);
			}

			//
			//	remove oldest message (message with the lowest id)
			//
			auto min_rec = tbl->min_record();
			if (!min_rec.empty()) {
				tbl->erase(min_rec.key(), tag);
			}
		}
		else {
			tbl->insert(cyng::table::key_generator(static_cast<std::uint64_t>(tbl->size()))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, static_cast<std::uint8_t>(level), msg)
				, 1, tag);
		}
	}

	void insert_ts_event(cyng::store::db& db
		, boost::uuids::uuid tag
		, std::string const& account
		, std::string const& evt
		, cyng::object obj)
	{
		db.access([&](cyng::store::table* tbl)->void {
			insert_ts_event(tbl, tag, account, evt, obj);
		}, cyng::store::write_access("_TimeSeries"));
	}

	void insert_ts_event(cyng::store::table* tbl
		, boost::uuids::uuid tag
		, std::string const& account
		, std::string const& evt
		, cyng::object obj)
	{
		//
		//	upper limit is 256 entries
		//
		if (tbl->size() > 255)
		{
			auto max_rec = tbl->max_record();
			if (!max_rec.empty()) {

				//	get next message id
				auto next_idx = cyng::value_cast<std::uint64_t>(max_rec["id"], 0u);

				tbl->insert(cyng::table::key_generator(++next_idx)
					, cyng::table::data_generator(std::chrono::system_clock::now()
						, tag
						, account
						, evt
						, cyng::io::to_str(obj))
					, 1, tag);
			}

			//
			//	remove oldest message (message with the lowest id)
			//
			auto min_rec = tbl->min_record();
			if (!min_rec.empty()) {
				tbl->erase(min_rec.key(), tag);
			}
		}
		else {
			tbl->insert(cyng::table::key_generator(static_cast<std::uint64_t>(tbl->size()))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, tag
					, account
					, evt
					, cyng::io::to_str(obj))
				, 1, tag);
		}
	}


	void insert_lora_uplink(cyng::store::db& db
		, std::chrono::system_clock::time_point tp
		, cyng::mac64 devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin)
	{
		db.access([&](cyng::store::table* tbl, cyng::store::table const* tbl_cfg)->void {
			auto rec = tbl_cfg->lookup(cyng::table::key_generator("max-messages"));
			const std::uint64_t max_messages = (!rec.empty())
				? cyng::value_cast<std::uint64_t>(rec["value"], 1000u)
				: 1000u
				;

			insert_lora_uplink(tbl, tp, devEUI, FPort, FCntUp, ADRbit, MType, FCntDn, customerID, payload, tag, origin, max_messages);
		}	, cyng::store::write_access("_LoRaUplink")
			, cyng::store::read_access("_Config"));

	}

	void insert_lora_uplink(cyng::store::table* tbl
		, std::chrono::system_clock::time_point tp
		, cyng::mac64 devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin
		, std::uint64_t max_messages)
	{
		//
		//	upper limit is 1000 messages
		//
		if (tbl->size() > max_messages)
		{
			auto max_rec = tbl->max_record();
			if (!max_rec.empty()) {

				//	get next message id
				auto next_idx = cyng::value_cast<std::uint64_t>(max_rec["id"], 0u);

				tbl->insert(cyng::table::key_generator(++next_idx)
					, cyng::table::data_generator(tp, devEUI, FPort, FCntUp, ADRbit, MType, FCntDn, customerID, payload, tag)
					, 1, origin);
			}

			//
			//	remove oldest message (message with the lowest id)
			//
			auto min_rec = tbl->min_record();
			if (!min_rec.empty()) {
				tbl->erase(min_rec.key(), origin);
			}
		}
		else {
			tbl->insert(cyng::table::key_generator(static_cast<std::uint64_t>(tbl->size()))
				, cyng::table::data_generator(tp, devEUI, FPort, FCntUp, ADRbit, MType, FCntDn, customerID, payload, tag)
				, 1, origin);
		}
	}


	cyng::table::record connection_lookup(cyng::store::table* tbl, cyng::table::key_type&& key)
	{
		cyng::table::record rec = tbl->lookup(key);
		if (!rec.empty())
		{
			return rec;
		}
		std::reverse(key.begin(), key.end());
		return tbl->lookup(key);

		//
		//	implementation note: C++20 will provide reverse_copy<>().
		//
	}

	bool connection_erase(cyng::store::table* tbl, cyng::table::key_type&& key, boost::uuids::uuid tag)
	{
		if (tbl->erase(key, tag))	return true;
		std::reverse(key.begin(), key.end());
		return tbl->erase(key, tag);
	}

	bool is_connection_auto_login(std::uint64_t cfg)
	{
		return (cfg & SMF_CONNECTION_AUTO_LOGIN) == SMF_CONNECTION_AUTO_LOGIN;
	}
	bool is_connection_auto_enabled(std::uint64_t cfg)
	{
		return (cfg & SMF_CONNECTION_AUTO_ENABLED) == SMF_CONNECTION_AUTO_ENABLED;
	}
	bool is_connection_superseed(std::uint64_t cfg)
	{
		return (cfg & SMF_CONNECTION_SUPERSEDED) == SMF_CONNECTION_SUPERSEDED;
	}
	bool is_generate_time_series(std::uint64_t cfg)
	{
		return (cfg & SMF_GENERATE_TIME_SERIES) == SMF_GENERATE_TIME_SERIES;
	}
	bool is_catch_meters(std::uint64_t cfg)
	{
		return (cfg & SMF_GENERATE_CATCH_METERS) == SMF_GENERATE_CATCH_METERS;
	}
	bool is_catch_lora(std::uint64_t cfg)
	{
		return (cfg & SMF_GENERATE_CATCH_LORA) == SMF_GENERATE_CATCH_LORA;
	}

	bool set_connection_auto_login(std::atomic<std::uint64_t>& cfg, bool b)
	{
		return is_connection_auto_login(b
			? cfg.fetch_or(SMF_CONNECTION_AUTO_LOGIN)
			: cfg.fetch_and(~SMF_CONNECTION_AUTO_LOGIN));
	}

	bool set_connection_auto_enabled(std::atomic<std::uint64_t>& cfg, bool b)
	{
		return is_connection_auto_enabled(b
			? cfg.fetch_or(SMF_CONNECTION_AUTO_ENABLED)
			: cfg.fetch_and(~SMF_CONNECTION_AUTO_ENABLED));
	}

	bool set_connection_superseed(std::atomic<std::uint64_t>& cfg, bool b)
	{
		return is_connection_superseed(b
			? cfg.fetch_or(SMF_CONNECTION_SUPERSEDED)
			: cfg.fetch_and(~SMF_CONNECTION_SUPERSEDED));
	}

	bool set_generate_time_series(std::atomic<std::uint64_t>& cfg, bool b)
	{
		return is_generate_time_series(b
			? cfg.fetch_or(SMF_GENERATE_TIME_SERIES)
			: cfg.fetch_and(~SMF_GENERATE_TIME_SERIES));
	}

	bool set_catch_meters(std::atomic<std::uint64_t>& cfg, bool b)
	{
		return is_catch_meters(b
			? cfg.fetch_or(SMF_GENERATE_CATCH_METERS)
			: cfg.fetch_and(~SMF_GENERATE_CATCH_METERS));
	}

	bool set_catch_lora(std::atomic<std::uint64_t>& cfg, bool b)
	{
		return is_catch_lora(b
			? cfg.fetch_or(SMF_GENERATE_CATCH_LORA)
			: cfg.fetch_and(~SMF_GENERATE_CATCH_LORA));
	}

}



