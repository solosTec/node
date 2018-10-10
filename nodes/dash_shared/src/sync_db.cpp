/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "sync_db.h"
#include <cyng/table/meta.hpp>

namespace node 
{
	void create_cache(cyng::logging::log_ptr logger, cyng::store::db& db)
	{
		CYNG_LOG_TRACE(logger, "create cache tables");

		if (!db.create_table(cyng::table::make_meta_table<1, 9>("TDevice",
			{ "pk", "name", "pwd", "msisdn", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_UINT32 },
			{ 36, 128, 16, 128, 512, 64, 64, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TDevice");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 15>("TGateway", { "pk"	//	primary key
			, "serverId"	//	(1) Server-ID (i.e. 0500153B02517E)
			, "manufacturer"	//	(2) manufacturer (i.e. EMH)
			, "made"	//	(4) production date
			, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
			, "ifService"	//	(7) MAC of service interface
			, "ifData"	//	(8) MAC od data interface
			, "pwdDef"	//	(9) Default PW
			, "pwdRoot"	//	(10) root PW
			, "mbus"	//	(11) W-Mbus ID (i.e. A815408943050131)
			, "userName"	//	(12)
			, "userPwd"	//	(13)
			, "name"	//	IP-T/device name
			, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
			, "vFirmware"	//	(5) firmwareversion (i.e. 11600000)
			, "online"	//	(14)
			},
			{ cyng::TC_UUID		//	pk
			, cyng::TC_STRING	//	server id
			, cyng::TC_STRING	//	manufacturer
			, cyng::TC_TIME_POINT	//	production data
			, cyng::TC_STRING	//	Fabriknummer/serial number (i.e. 06441734)
			, cyng::TC_MAC48	//	MAC of service interface
			, cyng::TC_MAC48	//	MAC od data interface
			, cyng::TC_STRING	//	Default PW
			, cyng::TC_STRING	//	root PW
			, cyng::TC_STRING	//	W-Mbus ID (i.e. A815408943050131)
			, cyng::TC_STRING	//	operator
			, cyng::TC_STRING	//	operator
								//	--- dynamic part:
			, cyng::TC_STRING	//	IP-T/device name
			, cyng::TC_STRING	//	model
			, cyng::TC_STRING	//	firmware
			, cyng::TC_BOOL		//	on/offline
			},
			{ 36		//	pk
			, 23	//	server id
			, 64	//	manufacturer
			, 0		//	production date
			, 8		//	serial
			, 18	//	MAC
			, 18	//	MAC
			, 32	//	PW
			, 32	//	PW
			, 16	//	M-Bus
			, 32	//	PW
			, 32	//	PW
			, 128	//	IP-T/device name
			, 64	//	model
			, 64	//	firmware
			, 0		//	bool online/offline
			})))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TGateway");
		}

		//	https://www.thethingsnetwork.org/docs/lorawan/address-space.html#devices
		//	DevEUI - 64 bit end-device identifier, EUI-64 (unique)
		//	DevAddr - 32 bit device address (non-unique)
		if (!db.create_table(cyng::table::make_meta_table<1, 7>("TLoRaDevice",
			{ "pk"
			, "DevEUI"
			, "AESKey"		// 256 Bit
			, "driver"
			, "activation"	//	OTAA/ABP
			, "DevAddr"		//	32 bit device address (non-unique)
			, "AppEUI"		//	64 bit application identifier, EUI-64 (unique)
			, "GatewayEUI"	//	64 bit gateway identifier, EUI-64 (unique)
			},
			{ cyng::TC_UUID
			, cyng::TC_MAC64	//	DevEUI
			, cyng::TC_STRING	//	AESKey
			, cyng::TC_STRING	//	driver
			, cyng::TC_BOOL		//	activation
			, cyng::TC_UINT32	//	DevAddr
			, cyng::TC_MAC64	//	AppEUI
			, cyng::TC_MAC64	//	GatewayEUI
			},
			{ 36, 0, 64, 32, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table TLoRaDevice");
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

		if (!db.create_table(cyng::table::make_meta_table<1, 12>("_Session", { "tag"	//	client session - primary key [uuid]
			, "local"	//	[object] local peer object (hold session reference)
			, "remote"	//	[object] remote peer object (if session connected)
			, "peer"	//	[uuid] remote peer
			, "device"	//	[uuid] - owner of the session
			, "name"	//	[string] - account
			, "source"	//	[uint32] - ipt source id (unique)
			, "loginTime"	//	last login time
			, "rtag"	//	[uuid] client session if connected
			, "layer"	//	[string] protocol layer
			, "rx"		//	[uint64] received bytes (from device)
			, "sx"		//	[uint64] sent bytes (to device)
			, "px"		//	[uint64] sent push data (to push target)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT64, cyng::TC_UINT64, cyng::TC_UINT64 },
			{ 36, 0, 0, 36, 36, 64, 0, 0, 36, 16, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Session");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 9>("_Target", { "channel"	//	name - primary key
			, "tag"		//	[uuid] owner session - primary key 
			, "peer"	//	[uuid] peer of owner
			, "name"	//	[uint32] - target id
			, "device"	//	[uuid] - owner of target
			, "account"	//	[string] - name of target owner
			, "pSize"	//	[uint16] - packet size
			, "wSize"	//	[uint8] - window size
			, "regTime"	//	registration time
			, "px"		//	incoming data
			},
			{ cyng::TC_UINT32, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT16, cyng::TC_UINT8, cyng::TC_TIME_POINT, cyng::TC_UINT64 },
			{ 0, 36, 36, 64, 36, 64, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Target");
		}

		if (!db.create_table(cyng::table::make_meta_table<2, 7>("_Connection",
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

		if (!db.create_table(cyng::table::make_meta_table<1, 8>("_Cluster", { "tag"	//	client session - primary key [uuid]
			, "class"
			, "loginTime"	//	last login time
			, "version"
			, "clients"	//	client counter
			, "ping"	//	ping time
			, "ep"		//	remote endpoint
			, "pid"		//	process id
			, "self"	//	"session" - surrogate
			},
			{ cyng::TC_UUID			//	tag (pk)
			, cyng::TC_STRING		//	class
			, cyng::TC_TIME_POINT	//	loginTime
			, cyng::TC_VERSION		//	version
			, cyng::TC_UINT64		//	clients
			, cyng::TC_MICRO_SECOND		//	ping
			, cyng::TC_IP_TCP_ENDPOINT	//	ep
			, cyng::TC_INT64	//	pid
			, cyng::TC_STRING	//	self == "session"
			},
			{ 36, 0, 32, 0, 0, 0, 0, 0, 0 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Cluster");
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 1>("_Config", { "name"	//	parameter name
			, "value"	//	parameter value
			},
			{ cyng::TC_STRING, cyng::TC_STRING },
			{ 64, 128 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *Config");
		}
		else
		{
			//
			//	set initial value
			//
			//db.insert("_Config", cyng::table::key_generator("cpu:load"), cyng::table::data_generator(0.0), 0, bus_->vm_.tag());
		}

		if (!db.create_table(cyng::table::make_meta_table<1, 3>("_SysMsg", { "id"	//	message number
			, "ts"	//	timestamp
			, "severity"
			, "msg"	//	message text
			},
			{ cyng::TC_UINT64, cyng::TC_TIME_POINT, cyng::TC_UINT8, cyng::TC_STRING },
			{ 0, 0, 0, 128 })))
		{
			CYNG_LOG_FATAL(logger, "cannot create table *SysMsg");
		}

		CYNG_LOG_INFO(logger, db.size() << " tables created");


	}

	void clear_cache(cyng::store::db& db, boost::uuids::uuid tag)
	{

		db.clear("TDevice", tag);
		db.clear("TGateway", tag);
		db.clear("TMeter", tag);
		db.clear("_Session", tag);
		db.clear("_Target", tag);
		db.clear("_Connection", tag);
		db.clear("_Cluster", tag);
		db.clear("_Config", tag);
		db.insert("_Config", cyng::table::key_generator("cpu:load"), cyng::table::data_generator(0.0), 0, tag);
		//cache_.clear("_SysMsg", bus_->vm_.tag());
	}
}
