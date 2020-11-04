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
#include <cyng/numeric_cast.hpp>

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

		CYNG_LOG_TRACE(logger, "create "
			<< cache::tables_.size()
			<< " cache tables");

		for (auto const& tbl : cache::tables_) {
			if (!tbl.custom_) {
				if (!create_table(db, tbl.name_)) {
					CYNG_LOG_FATAL(logger, "cannot create table: " << tbl.name_);
				}
				else {
					CYNG_LOG_TRACE(logger, "create table: " << tbl.name_);
				}
			}
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
				, boost::uuids::string_generator()("8d04b8e0-0faf-44ea-b32b-8405d407f2c1")	//	reference to TGateway 8d04b8e0-0faf-44ea-b32b-8405d407f2c1
				, "SML")
			, 5
			, tag);

		{
			//	segw.ch
			auto const host = cyng::make_address("138.201.95.180");
			auto const ep = boost::asio::ip::tcp::endpoint{ host, 4004 };

			db.insert("TIECBridge"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator(ep.address()
					, ep.port()
					, true
					, std::chrono::seconds(60))
				, 12
				, tag);
		}

		db.insert("TGWSnapshot"
			, cyng::table::key_generator(tag)
			, cyng::table::data_generator("0500153B02517E"
				, "works like a charm"
				, std::chrono::system_clock::now())
			, 5
			, tag);

		db.insert("TNodeNames"
			, cyng::table::key_generator(tag)
			, cyng::table::data_generator("master")
			, 3
			, tag);

		{
			//	segw.ch
			auto const host = cyng::make_address("138.201.95.180");
			auto const ep = boost::asio::ip::tcp::endpoint{ host, 6006 };
			db.insert("_IECUplink"
				, cyng::table::key_generator(2)
				, cyng::table::data_generator(std::chrono::system_clock::now(), "DEBUG EVENT", ep, tag)
				, 3
				, tag);
		}

#endif

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
			auto rec = tbl_cfg->lookup(cyng::table::key_generator("max-LoRa-records"));
			const std::uint64_t max_messages = (!rec.empty())
				? cyng::numeric_cast<std::uint64_t>(rec["value"], 500u)
				: 500u
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
		//	upper limit is defined in "max-LoRa-records"
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

	bool insert_wmbus_uplink(cyng::store::db& db
		, std::chrono::system_clock::time_point tp
		, std::string const& srv_id
		, std::uint8_t medium
		, std::string  manufacturer
		, std::uint8_t frame_type
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin)
	{
		bool r{ false };
		db.access([&](cyng::store::table* tbl, cyng::store::table const* tbl_cfg)->void {
			auto rec = tbl_cfg->lookup(cyng::table::key_generator("max-wMBus-records"));
			const std::uint64_t max_messages = (!rec.empty())
				? cyng::numeric_cast<std::uint64_t>(rec["value"], 500u)
				: 500u
				;

			r = insert_wmbus_uplink(tbl, tp, srv_id, medium, manufacturer, frame_type, payload, tag, origin, max_messages);

		}	, cyng::store::write_access("_wMBusUplink")
			, cyng::store::read_access("_Config"));

		return r;

	}

	bool insert_wmbus_uplink(cyng::store::table* tbl
		, std::chrono::system_clock::time_point tp
		, std::string const& srv_id
		, std::uint8_t medium
		, std::string  manufacturer
		, std::uint8_t frame_type
		, std::string const& payload
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin
		, std::uint64_t max_messages)
	{
		//
		//	upper limit is defined in "max-wMBus-records"
		//
		if (tbl->size() > max_messages)
		{
			//
			//	remove oldest message (message with the lowest id)
			//
			auto min_rec = tbl->min_record();
			if (!min_rec.empty()) {
				tbl->erase(min_rec.key(), origin);
			}

			auto max_rec = tbl->max_record();
			if (!max_rec.empty()) {

				//	get next message id
				auto next_idx = cyng::value_cast<std::uint64_t>(max_rec["id"], 0u);

				return tbl->insert(cyng::table::key_generator(++next_idx)
					, cyng::table::data_generator(tp, srv_id, medium, manufacturer, frame_type, payload, tag)
					, 1, origin);
			}

		}

		return tbl->insert(cyng::table::key_generator(static_cast<std::uint64_t>(tbl->size()))
				, cyng::table::data_generator(tp, srv_id, medium, manufacturer, frame_type, payload, tag)
				, 1, origin);
	}

	bool insert_iec_uplink(cyng::store::db& db
		, std::chrono::system_clock::time_point tp
		, std::string const& evt
		, boost::asio::ip::tcp::endpoint ep
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin)
	{
		bool r{ false };
		db.access([&](cyng::store::table* tbl, cyng::store::table const* tbl_cfg)->void {
			auto rec = tbl_cfg->lookup(cyng::table::key_generator("max-wMBus-records"));
			const std::uint64_t max_messages = (!rec.empty())
				? cyng::numeric_cast<std::uint64_t>(rec["value"], 500u)
				: 500u
				;

			r = insert_iec_uplink(tbl, tp, evt, ep, tag, origin, max_messages);

		}	, cyng::store::write_access("_IECUplink")
			, cyng::store::read_access("_Config"));

		return r;
	}

	bool insert_iec_uplink(cyng::store::table* tbl
		, std::chrono::system_clock::time_point tp
		, std::string const& evt
		, boost::asio::ip::tcp::endpoint ep
		, boost::uuids::uuid tag
		, boost::uuids::uuid origin
		, std::uint64_t max_messages)
	{
		//
		//	upper limit is defined in "max-wMBus-records"
		//
		if (tbl->size() > max_messages)
		{
			//
			//	remove oldest message (message with the lowest id)
			//
			auto min_rec = tbl->min_record();
			if (!min_rec.empty()) {
				tbl->erase(min_rec.key(), origin);
			}

			auto max_rec = tbl->max_record();
			if (!max_rec.empty()) {

				//	get next message id
				auto next_idx = cyng::value_cast<std::uint64_t>(max_rec["id"], 0u);

				return tbl->insert(cyng::table::key_generator(++next_idx)
					, cyng::table::data_generator(tp, evt, ep, tag)
					, 1, origin);
			}

		}

		return tbl->insert(cyng::table::key_generator(static_cast<std::uint64_t>(tbl->size()))
			, cyng::table::data_generator(tp, evt, ep, tag)
			, 1, origin);
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

	/**
	 * Initialize all used table names
	 */
	const std::array<cache::tbl_descr, 23>	cache::tables_ =
	{
		tbl_descr{"TDevice", false},
		tbl_descr{"TGateway", false},
		tbl_descr{"TLoRaDevice", false},
		tbl_descr{"TMeter", false},
		tbl_descr{"TMeterwMBUS", false},
		tbl_descr{"TLL", false},
		tbl_descr{"TGUIUser", false},
		tbl_descr{"TGWSnapshot", false},
		tbl_descr{"TNodeNames", false},
		tbl_descr{"TIECBridge", false},
		tbl_descr{"_Session", false},
		tbl_descr{"_Channel", true},	//	custom
		tbl_descr{"_Target", false},
		tbl_descr{"_Connection", false},
		tbl_descr{"_Cluster", false},
		tbl_descr{"_Config", false},
		tbl_descr{"_SysMsg", false},
		tbl_descr{"_TimeSeries", false},
		tbl_descr{"_LoRaUplink", false},
		tbl_descr{"_wMBusUplink", false},
		tbl_descr{"_IECUplink", false},
		tbl_descr{"_CSV", false},
		tbl_descr{"_Broker", false}	//	broker 
	};

	cache::cache(cyng::store::db& db, boost::uuids::uuid tag)
		: db_(db)
		, tag_(tag)
		, sys_conf_(0)
	{}

	boost::uuids::uuid const cache::get_tag() const
	{
		return tag_;
	}

	void cache::init()
	{
		set_cfg("startup", std::chrono::system_clock::now());

		set_cfg("sys-version", NODE::version_string);
		set_cfg("boost-version", NODE_BOOST_VERSION);
		set_cfg("ssl-version", NODE_SSL_VERSION);


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

	void cache::init_sys_cfg(
		bool auto_login,
		bool auto_enabled,
		bool supersede,
		bool gw_cache,
		bool generate_time_series,
		bool catch_meters,
		bool catch_lora) 
	{
		set_connection_auto_login(auto_login);
		set_connection_auto_enabled(auto_enabled);
		set_connection_superseed(supersede);
		set_gw_cache_enabled(gw_cache);

		/**
		 * Turn generating time series on or off
		 *
		 * @return previous value
		 */
		set_generate_time_series(generate_time_series);
		set_catch_meters(catch_meters);
		set_catch_lora(catch_lora);

		//
		//	backup to cache
		//
		set_cfg("connection-auto-login", auto_login);
		set_cfg("connection-auto-enabled", auto_enabled);
		set_cfg("connection-superseed", supersede);
		set_cfg("gw-cache", gw_cache);
		set_cfg("generate-time-series", generate_time_series);
		set_cfg("catch-meters", catch_meters);
		set_cfg("catch-lora", catch_lora);
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
	bool cache::is_gw_cache_enabled() const
	{
		return (sys_conf_ & SMF_GW_CACHE_ENABLED) == SMF_GW_CACHE_ENABLED;
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

	bool cache::set_gw_cache_enabled(bool b)
	{
		b
			? sys_conf_.fetch_or(SMF_GW_CACHE_ENABLED)
			: sys_conf_.fetch_and(~SMF_GW_CACHE_ENABLED);
		return is_gw_cache_enabled();
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

	std::uint64_t cache::get_max_LoRa_records()
	{
		return get_cfg<std::uint64_t>("max-LoRa-records", 500u);
	}

	std::uint64_t cache::get_max_wMBus_records()
	{
		return get_cfg<std::uint64_t>("max-wMBus-records", 500u);
	}

}



