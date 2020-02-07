/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "db.h"
#include <smf/shared/db_schemes.h>
#include <NODE_project_info.h>
#include <cyng/table/meta.hpp>
#include <cyng/intrinsics/traits/tag.hpp>
#include <cyng/intrinsics/traits.hpp>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>

#include <algorithm>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/process/environment.hpp>
#include <boost/algorithm/string.hpp>


namespace node 
{
	void create_tables(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::uuids::uuid tag)
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

		if (!create_table(db, "TGUIUser"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TGUIUser");
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
				cyng::TC_UINT32, 
				cyng::TC_UINT32, 
				cyng::TC_UINT32, 
				cyng::TC_UUID, 
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(cyng::traits::predef_type_code::PREDEF_SESSION),
#else
				cyng::traits::PREDEF_SESSION,
#endif
				cyng::TC_UUID, 
				cyng::TC_UINT16, 
				cyng::TC_UINT32, 
				cyng::TC_UINT64, 
				cyng::TC_STRING, 
				cyng::TC_STRING
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

		}

		if (!create_table(db, "_Config"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Config");
		}
		else
		{

		}

		if (!create_table(db, "_SysMsg"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _SysMsg");
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

	void insert_ts_event(cyng::store::table* tbl
		, boost::uuids::uuid tag
		, std::string const& account
		, std::string const& evt
		, cyng::object obj
		, std::uint64_t max_events)
	{
		//
		//	upper limit is 256 entries
		//
		if (tbl->size() > max_events)
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

	cyng::table::record lookup_meter(cyng::store::table const* tbl, std::string const& ident, boost::uuids::uuid gw_tag)
	{
		cyng::table::record result = cyng::table::make_empty_record(tbl->meta_ptr());

		//
		//	"ident" + "gw" have to be unique
		//
		tbl->loop([&](cyng::table::record const& rec) -> bool {

			const auto cmp_ident = cyng::value_cast<std::string>(rec["ident"], "");

			//
			//	test if meter "ident" is matching
			//
			if (boost::algorithm::equals(ident, cmp_ident)) {

				auto const meter_gw_tag = cyng::value_cast(rec["gw"], boost::uuids::nil_uuid());

				//
				//	test if the matching meter id is from a different gateway
				//	abort loop if the same gw key is used
				//
				if (meter_gw_tag == gw_tag) {
					result = rec;
					return false;	//	stop
				}
			}

			//	continue search
			return true;
		});

		return result;
	}

	cache::cache(cyng::store::db& db, boost::uuids::uuid tag)
		: db_(db)
		, tag_(tag)
		, sys_conf_(0)
	{}

	boost::uuids::uuid const cache::get_tag() const
	{
		return tag_;
	}

	void cache::init(std::string country_code
		, std::string language_code
		, boost::filesystem::path stat_dir
		, std::uint64_t max_messages
		, std::uint64_t max_events
		, std::chrono::seconds heartbeat)
	{
		set_cfg("startup", std::chrono::system_clock::now());

		set_cfg("connection-auto-login", is_connection_auto_login());
		set_cfg("connection-auto-enabled", is_connection_auto_enabled());
		set_cfg("connection-superseed", is_connection_superseed());
		set_cfg("generate-time-series", is_generate_time_series());
		set_cfg("catch-meters", is_catch_meters());
		set_cfg("catch-lora", is_catch_lora());

		set_cfg("generate-time-series-dir", stat_dir.string());
		set_cfg("country-code", country_code);
		set_cfg("country-code-default", country_code);
		set_cfg("language-code", language_code);
		set_cfg("language-code-default", language_code);
		set_cfg("max-messages", max_messages);
		set_cfg("max-messages-default", max_messages);
		set_cfg("max-events", max_events);
		set_cfg("max-events-default", max_events);
		set_cfg("cluster-heartbeat", heartbeat);


		//	get hostname
		boost::system::error_code ec;
		auto host_name = boost::asio::ip::host_name(ec);
		if (ec) {
			host_name = ec.message();
		}
		set_cfg("host-name", host_name);

		set_cfg("smf-version", NODE_SUFFIX);

		//
		//	system message
		//
		std::stringstream ss;
		ss
			<< "startup master node "
			<< tag_
			<< " on "
			<< host_name
			;
		insert_msg(cyng::logging::severity::LEVEL_INFO, ss.str(), tag_);

	}

	bool cache::merge_cfg(std::string name, cyng::object&& val)
	{
		bool r{ false };
		db_.access([&](cyng::store::table* tbl) {

			r = tbl->merge(cyng::table::key_generator(name)
				, cyng::table::data_generator(std::move(val))
				, 1u	//	only needed for insert operations
				, tag_);

			}, cyng::store::write_access("_Config"));

		return r;
	}

	void cache::read_table(std::string const& name, std::function<void(cyng::store::table const*)> f)
	{
		db_.access([f](cyng::store::table const* tbl) {
			f(tbl);
			}, cyng::store::read_access(name));
	}

	void cache::read_tables(std::string const& t1, std::string const& t2, std::function<void(cyng::store::table const*, cyng::store::table const*)> f)
	{
		db_.access([f](cyng::store::table const* tbl1, cyng::store::table const* tbl2) {
			f(tbl1, tbl2);
			}, cyng::store::read_access(t1), cyng::store::read_access(t2));
	}

	void cache::write_table(std::string const& name, std::function<void(cyng::store::table*)> f)
	{
		db_.access([f](cyng::store::table* tbl) {
			f(tbl);
			}, cyng::store::write_access(name));
	}

	void cache::write_tables(std::string const& t1, std::string const& t2, std::function<void(cyng::store::table*, cyng::store::table*)> f)
	{
		db_.access([f](cyng::store::table* tbl1, cyng::store::table* tbl2) {
			f(tbl1, tbl2);
			}, cyng::store::write_access(t1), cyng::store::write_access(t2));
	}

	void cache::clear_table(std::string const& name) {
		db_.clear(name, tag_);
	}

	void cache::loop(std::string const& name, std::function<bool(cyng::table::record const&)> f)
	{
		db_.access([f](cyng::store::table const* tbl) {
			tbl->loop([f](cyng::table::record const& rec)->bool {
				return f(rec);
				});
			}, cyng::store::read_access(name));
	}


	bool cache::is_connection_auto_login() const
	{
		return (sys_conf_ & SMF_CONNECTION_AUTO_LOGIN) == SMF_CONNECTION_AUTO_LOGIN;
	}
	bool cache::is_connection_auto_enabled() const
	{
		return (sys_conf_ & SMF_CONNECTION_AUTO_ENABLED) == SMF_CONNECTION_AUTO_ENABLED;
	}
	bool cache::is_connection_superseed() const
	{
		return (sys_conf_ & SMF_CONNECTION_SUPERSEDED) == SMF_CONNECTION_SUPERSEDED;
	}
	bool cache::is_generate_time_series() const
	{
		return (sys_conf_ & SMF_GENERATE_TIME_SERIES) == SMF_GENERATE_TIME_SERIES;
	}
	bool cache::is_catch_meters() const
	{
		return (sys_conf_ & SMF_GENERATE_CATCH_METERS) == SMF_GENERATE_CATCH_METERS;
	}
	bool cache::is_catch_lora() const
	{
		return (sys_conf_ & SMF_GENERATE_CATCH_LORA) == SMF_GENERATE_CATCH_LORA;
	}

	bool cache::set_connection_auto_login(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_CONNECTION_AUTO_LOGIN)
			: sys_conf_.fetch_and(~SMF_CONNECTION_AUTO_LOGIN);
		return is_connection_auto_login();
	}

	bool cache::set_connection_auto_enabled(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_CONNECTION_AUTO_ENABLED)
			: sys_conf_.fetch_and(~SMF_CONNECTION_AUTO_ENABLED);
		return is_connection_auto_enabled();
	}

	bool cache::set_connection_superseed(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_CONNECTION_SUPERSEDED)
			: sys_conf_.fetch_and(~SMF_CONNECTION_SUPERSEDED);
		return is_connection_superseed();
	}

	bool cache::set_generate_time_series(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_GENERATE_TIME_SERIES)
			: sys_conf_.fetch_and(~SMF_GENERATE_TIME_SERIES);
		return is_generate_time_series();
	}

	bool cache::set_catch_meters(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_GENERATE_CATCH_METERS)
			: sys_conf_.fetch_and(~SMF_GENERATE_CATCH_METERS);
		return is_catch_meters();
	}

	bool cache::set_catch_lora(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_GENERATE_CATCH_LORA)
			: sys_conf_.fetch_and(~SMF_GENERATE_CATCH_LORA);
		return is_catch_lora();
	}

	void cache::create_master_record(boost::asio::ip::tcp::endpoint ep)
	{
		db_.insert("_Cluster"
			, cyng::table::key_generator(tag_)
			, cyng::table::data_generator("master"
				, std::chrono::system_clock::now()
				, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
				, 0u	//	no clients yet
				, std::chrono::microseconds(0)
				, ep
				, boost::this_process::get_id()
				, cyng::null())	//	no session instance
			, 1
			, tag_);

	}

	void cache::insert_msg(cyng::logging::severity level
		, std::string const& msg
		, boost::uuids::uuid tag)
	{
		//
		//	read max number of messages from config table
		//
		auto const max_messages = get_max_messages();

		write_table("_SysMsg",[&](cyng::store::table* tbl) {

			node::insert_msg(tbl, level, msg, tag, max_messages);

		});
	}

	//void cache::insert_ts_event(boost::uuids::uuid tag
	//	, std::string const& account
	//	, std::string const& evt
	//	, cyng::object obj)
	//{
	//	//
	//	//	read max number of events from config table
	//	//
	//	auto const max_events = get_max_events();

	//	write_table("_TimeSeries", [&](cyng::store::table* tbl_ts) {

	//		node::insert_ts_event(tbl_ts, tag, account, evt, obj, max_events);

	//	});
	//}

	std::chrono::seconds cache::get_cluster_hartbeat()
	{
		return get_cfg("cluster-heartbeat", std::chrono::seconds(57));
	}

	std::uint64_t cache::get_max_events()
	{
		return get_cfg<std::uint64_t>("max-events", 2000u);
	}

	std::uint64_t cache::get_max_messages()
	{
		return get_cfg<std::uint64_t>("max-messages", 1000u);
	}

}



