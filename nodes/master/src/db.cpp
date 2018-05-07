/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "db.h"
#include <NODE_project_info.h>
#include <cyng/table/meta.hpp>
#include <cyng/intrinsics/traits/tag.hpp>
#include <cyng/intrinsics/traits.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/process/environment.hpp>
#include <algorithm>

namespace node 
{
	void init(cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::uuids::uuid tag
		, boost::asio::ip::tcp::endpoint ep
		, std::chrono::seconds connection_open_timeout
		, std::chrono::seconds connection_close_timeout
		, bool connection_auto_login
		, bool connection_auto_enabled
		, bool connection_superseed)
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

		if (!db.create_table(cyng::table::make_meta_table<1, 9>("TDevice",
			{ "pk", "name", "pwd", "msisdn", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_UINT32 },
			{ 36, 128, 16, 128, 512, 64, 64, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TDevice");
		}
		else
		{
			//db.insert("TDevice"
			//	, cyng::table::key_generator(tag)
			//	, cyng::table::data_generator("master", "undefined", "0000", "synthetic test device", "smf", NODE_VERSION, true, std::chrono::system_clock::now(), 6u)
			//	, 72
			//	, tag);

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
		if (!db.create_table(cyng::table::make_meta_table<1, 13>("TGateway", { "pk"	//	primary key
				, "id"	//	(1) Server-ID (i.e. 0500153B02517E)
				, "manufacturer"	//	(2) manufacturer (i.e. EMH)
				, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
				, "made"	//	(4) production date
				, "vFirmware"	//	(5) firmwareversion (i.e. 11600000)
				, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
				, "ifService"	//	(7) MAC of service interface
				, "ifData"	//	(8) MAC of data interface
				, "pwdDef"	//	(9) Default PW
				, "pwdRoot"	//	(10)
				, "mbus"	//	(11)
				, "userName"	//	(12)
				, "userPwd"	//	(13)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_MAC48, cyng::TC_MAC48, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 23, 64, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TGateway");
		}
		else
		{
			db.insert("TGateway"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator("0500153B02517E"
					, "EMH"
					, "Variomuc ETHERNET"
					, std::chrono::system_clock::now()
					, "11600000"
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
		}


		if (!db.create_table(cyng::table::make_meta_table<1, 9>("TMeter", { "pk"
			, "ident"	//	ident nummer (i.e. 1EMH0006441734)
			, "manufacturer"
			, "factoryNr"	//	fabrik nummer (i.e. 06441734)
			, "age"	//	production data
			, "vParam"	//	parametrierversion (i.e. 16A098828.pse)
			, "vFirmware"	//	firmwareversion (i.e. 11600000)
			, "item"	//	 artikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
			, "class"	//	Metrological Class: A, B, C, Q3/Q1, ...
			, "source"	//	source client (UUID)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UUID },
			{ 36, 64, 64, 8, 0, 64, 64, 128, 8, 36 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TMeter");
		}

		//
		//	TLL table - leased lines
		//
		//	(1) first (UUID) - pk
		//	(2) second (UUID) - pk
		//	+-----------------
		//	(3) description [std::string]
		//	(4) creation-time [std::chrono::system_clock::time_stamp]

		if (!db.create_table(cyng::table::make_meta_table<2, 3>("TLL",
			{ "first"
			, "second"
			, "descr"
			, "enabled"
			, "creationTime" },
			{ cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
			{ 36, 36, 128, 0, 0 })))
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
		if (!db.create_table(cyng::table::make_meta_table<1, 12>("*Session", { "tag"	//	client session - primary key [uuid]
			, "local"	//	[object] local peer object (hold session reference)
			, "remote"	//	[object] remote peer object (if session connected)
			, "peer"	//	[uuid] remote peer
			, "device"	//	[uuid] - owner of the session
			, "name"	//	[string] - account
			, "source"	//	[uint32] - ipt source id (unique)
			, "loginTime"	//	[tp] last login time
			, "rtag"	//	[uuid] client session if connected
			, "layer"	//	[string] protocol layer
			, "rx"		//	[uint64] received bytes (from device)
			, "sx"		//	[uint64] sent bytes (to device)
			, "px"		//	[uint64] sent push data (to push target)
			},
			{ cyng::TC_UUID, cyng::traits::PREDEF_SESSION, cyng::traits::PREDEF_SESSION, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT64, cyng::TC_UINT64, cyng::TC_UINT64 },
			{ 36, 0, 0, 36, 36, 64, 0, 0, 36, 16, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Session");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 9>("*Target", { "channel"	//	[uint32] - target id: primary key
			, "tag"		//	[uuid] owner session - primary key 
			, "peer"	//	[uuid] peer of owner
			, "name"	//	[string] target name
			, "device"	//	[uuid] - owner of target
			, "account"	//	[string] - name of target owner
			, "pSize"	//	[uint16] - packet size
			, "wSize"	//	[uint8] - window size
			, "regTime"	//	[tp] registration time
			, "px"		//	incoming data
			},
			{ cyng::TC_UINT32, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT16, cyng::TC_UINT8, cyng::TC_TIME_POINT, cyng::TC_UINT64 },
			{ 0, 36, 36, 64, 36, 64, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Target");
		}

		//
		//	ack-time: the time interval (in seconds) in which a Push Data Transfer Response is expected
		//	after the transmission of the last character of a Push Data Transfer Request.
		//
		if (!db.create_table(cyng::table::make_meta_table<3, 6>("*Channel", 
			{ "channel"		//	[uint32] primary key 
			, "source"		//	[uint32] primary key 
			, "target"		//	[uint32] primary key 
			, "tag"			//	[uuid] remote target session tag
			, "peerTarget"	//	[object] target peer session
			, "peerChannel"	//	[uuid] owner/channel peer session
			, "pSize"		//	[uint16] - max packet size
			, "ackTime"		//	[uint32] - See description above
			, "count"		//	[size_t] target count
			},
			{	
				cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32, 
				cyng::TC_UUID, cyng::traits::PREDEF_SESSION, cyng::TC_UUID, cyng::TC_UINT16, cyng::TC_UINT32, cyng::TC_UINT64
			},
			{ 0, 0, 0, 0, 0, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Channel");
		}

		//
		//	All dial-up connections. Leased Lines have to be incorporated.
		//
		if (!db.create_table(cyng::table::make_meta_table<2, 7>("*Connection",
			{ "first"		//	[uuid] primary key 
			, "second"		//	[uuid] primary key 
			, "aName"		//	[string] caller
			, "bName"		//	[string] callee
			, "local"		//	[bool] true if local connection
			, "aLayer"		//	[string] protocol layer of caller
			, "bLayer"		//	[string] protocol layer of callee
			, "throughput"	//	[uint64] data throughput
			, "start"		//	[tp] start time
			},
			{
				cyng::TC_UUID, cyng::TC_UUID,
				cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT64, cyng::TC_TIME_POINT
			},
			{ 0, 0, 128, 128, 0, 16, 16, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Connection");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 8>("*Cluster", 
			{ "tag"			//	[uuid] client session - primary key 
			, "class"		//	[string] node class
			, "loginTime"	//	last login time
			, "version"
			, "clients"	//	client counter
			, "ping"	//	ping time
			, "ep"		//	remote endpoint
			, "pid"		//	process id
			, "self"	//	[object] session instance
			},
			{ cyng::TC_UUID			//	tag
			, cyng::TC_STRING		//	class
			, cyng::TC_TIME_POINT	//	loginTime
			, cyng::TC_VERSION		//	version
			, cyng::TC_UINT64		//	clients (dynamic)
			, cyng::TC_MICRO_SECOND	//	ping time (dynamic)
			, cyng::TC_IP_TCP_ENDPOINT	//	ep
			, cyng::TC_INT64		//	pid
			, cyng::traits::PREDEF_SESSION	//	self
			},
			{ 36, 0, 32, 0, 0, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Cluster");
		}
		else
		{
			db.insert("*Cluster"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator("master"
					, std::chrono::system_clock::now()
					, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
					, 0u	//	no clients yet
					, std::chrono::microseconds(0)
					, ep
					, boost::this_process::get_id())
				, 1
				, tag);

		}

		if (!db.create_table(cyng::table::make_meta_table<1, 1>("*Config", { "name"	//	parameter name
			, "value"	//	parameter value
			},
			{ cyng::TC_STRING, cyng::TC_STRING },
			{ 64, 128 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Config");
		}
		else
		{
			//db.insert("*Config", cyng::table::key_generator("startup"), cyng::table::data_generator(std::chrono::system_clock::now()), 1, tag);
			db.insert("*Config", cyng::table::key_generator("master-tag"), cyng::table::data_generator(tag), 1, tag);
			db.insert("*Config", cyng::table::key_generator("connection-open-timeout"), cyng::table::data_generator(connection_open_timeout), 1, tag);
			db.insert("*Config", cyng::table::key_generator("connection-close-timeout"), cyng::table::data_generator(connection_close_timeout), 1, tag);
			db.insert("*Config", cyng::table::key_generator("connection-auto-login"), cyng::table::data_generator(connection_auto_login), 1, tag);
			db.insert("*Config", cyng::table::key_generator("connection-auto-enabled"), cyng::table::data_generator(connection_auto_enabled), 1, tag);
			db.insert("*Config", cyng::table::key_generator("connection-superseed"), cyng::table::data_generator(connection_superseed), 1, tag);
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 3>("*SysMsg", { "id"	//	message number
			, "ts"	//	timestamp
			, "severity"	
			, "msg"	//	message text
			},
			{ cyng::TC_UINT64, cyng::TC_TIME_POINT, cyng::TC_UINT8, cyng::TC_STRING },
			{ 0, 0, 0, 128 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *SysMsg");
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
	}

	void insert_msg(cyng::store::db& db
		, cyng::logging::severity level
		, std::string const& msg
		, boost::uuids::uuid tag)
	{
		db.access([&](cyng::store::table* tbl)->void {
			insert_msg(tbl, level, msg, tag);
		}, cyng::store::write_access("*SysMsg"));

	}

	void insert_msg(cyng::store::table* tbl
		, cyng::logging::severity level
		, std::string const& msg
		, boost::uuids::uuid tag)
	{
		tbl->insert(cyng::table::key_generator(static_cast<std::uint64_t>(tbl->size()))
			, cyng::table::data_generator(std::chrono::system_clock::now()
			, static_cast<std::uint8_t>(level), msg)
			, 1, tag);

		if (tbl->size() > 1000)
		{
			tbl->clear(tag);
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


}



