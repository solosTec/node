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

namespace node 
{
	void init(cyng::logging::log_ptr logger, cyng::store::db& db, boost::uuids::uuid tag)
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
			//db.insert("TDevice", cyng::store::key_generator(tag), cyng::store::data_generator("name", "number", "descr", "id", "vFirmware", true, std::chrono::system_clock::now(), cyng::param_map_t()), 72);
			db.insert("TDevice"
				, cyng::table::key_generator(tag)
				, cyng::table::data_generator("master", "undefined", "0000", "synthetic test device", "smf", NODE_VERSION, true, std::chrono::system_clock::now(), 6u)
				, 72
				, tag);

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
				, "ifData"	//	(8) MAC od data interface
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
		if (!db.create_table(cyng::table::make_meta_table<1, 8>("*Session", { "tag"	//	client session - primary key [uuid]
			, "local"	//	[object] local peer object (hold session reference)
			, "remote"	//	[object] remote peer object (if session connected)
			, "peer"	//	[uuid] remote peer
			, "device"	//	[uuid] - owner of the session
			, "name"	//	[string] - account
			, "source"	//	[uint32] - ipt source id (unique)
			, "login-time"	//	last login time
			, "rtag"	//	[uuid] client session if connected
			},
			{ cyng::TC_UUID, cyng::traits::PREDEF_SESSION, cyng::traits::PREDEF_SESSION, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_UUID },
			{ 36, 0, 0, 36, 36, 64, 0, 0, 36 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Session");
		}

		if (!db.create_table(cyng::table::make_meta_table<2, 7>("*Target", { "name"	//	name - primary key
			, "tag"		//	[uuid] owner session - primary key 
			, "peer"	//	[uuid] peer of owner
			, "channel"	//	[uint32] - target id
			, "device"	//	[uuid] - owner of target
			, "p-size"	//	[uint16] - packet size
			, "w-size"	//	[uint8] - window size
			, "reg-time"	//	registration time
			, "px"		//	incoming data
			},
			{ cyng::TC_STRING, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_UINT32, cyng::TC_UUID, cyng::TC_UINT16, cyng::TC_UINT8, cyng::TC_TIME_POINT, cyng::TC_UINT64 },
			{ 64, 36, 36, 0, 36, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Target");
		}

		//
		//	ack-time: the time interval (in seconds) in which a Push Data Transfer Response is expected
		//	after the transmission of the last character of a Push Data Transfer Request.
		//
		if (!db.create_table(cyng::table::make_meta_table<3, 5>("*Channel", 
			{ "channel"		//	[uint32] primary key 
			, "source"		//	[uint32] primary key 
			, "target"		//	[uint32] primary key 
			, "tag"			//	[uuid] target session tag
			, "peer"		//	[session] target peer object
			, "p-size"		//	[uint16] - max packet size
			, "ack-time"	//	[uint32] - See description above
			, "count"		//	[size_t] target count
			},
			{	
				cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32, 
				cyng::TC_UUID, cyng::traits::PREDEF_SESSION, cyng::TC_UINT16, cyng::TC_UINT32,cyng::TC_UINT64
			},
			{ 0, 0, 0, 0, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Channel");
		}

	}

}



