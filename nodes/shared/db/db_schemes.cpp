/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <cyng/store/db.h>
#include "db_schemes.h"

#include <cyng/table/meta.hpp>

#include <boost/algorithm/string.hpp>

namespace node
{
	cyng::table::meta_table_ptr create_meta(std::string name)
	{
		if (boost::algorithm::equals(name, "TDevice")) {

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
			return cyng::table::make_meta_table<1, 9>(name,
				{ "pk"
				, "name"
				, "pwd"
				, "msisdn"
				, "descr"
				, "id"
				, "vFirmware"
				, "enabled"
				, "creationTime"
				, "query" },
				{ cyng::TC_UUID		//	pk (36)
				, cyng::TC_STRING	//	name (128)
				, cyng::TC_STRING	//	pwd	(32)
				, cyng::TC_STRING	//	msisdn (64)
				, cyng::TC_STRING	//	description (512)
				, cyng::TC_STRING	//	identifier (64)
				, cyng::TC_STRING	//	vFirmware (64)
				, cyng::TC_BOOL		//	enabled
				, cyng::TC_TIME_POINT	//	creationTime
				, cyng::TC_UINT32	//	query
				},
				{ 36, 128, 32, 64, 512, 64, 64, 0, 0, 0 });
		}
		else if (boost::algorithm::equals(name, "TLoRaDevice")) {

			//	https://www.thethingsnetwork.org/docs/lorawan/address-space.html#devices
			//	DevEUI - 64 bit end-device identifier, EUI-64 (unique)
			//	DevAddr - 32 bit device address (non-unique)
			return cyng::table::make_meta_table<1, 7>(name,
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
				{ 36, 0, 64, 32, 0, 0, 0, 0 });
		}
		else if (boost::algorithm::equals(name, "_Session")) {

			return cyng::table::make_meta_table<1, 12>(name, { "tag"	//	client session - primary key [uuid]
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
				{ 36, 0, 0, 36, 36, 64, 0, 0, 36, 16, 0, 0, 0 });
		}
		else if (boost::algorithm::equals(name, "_Target")) {

			return cyng::table::make_meta_table<1, 10>(name, { "channel"	//	name - primary key
				, "tag"		//	[uuid] owner session - primary key 
				, "peer"	//	[uuid] peer of owner
				, "name"	//	[uint32] - target id
				, "device"	//	[uuid] - owner of target
				, "account"	//	[string] - name of target owner
				, "pSize"	//	[uint16] - packet size
				, "wSize"	//	[uint8] - window size
				, "regTime"	//	registration time
				, "px"		//	incoming data
				, "counter"	//	message counter
				},
				{ cyng::TC_UINT32, cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT16, cyng::TC_UINT8, cyng::TC_TIME_POINT, cyng::TC_UINT64, cyng::TC_UINT64 },
				{ 0, 36, 36, 64, 36, 64, 0, 0, 0, 0, 0 });

		}
		else if (boost::algorithm::equals(name, "TMeter")) {

			return cyng::table::make_meta_table<1, 11>(name, { "pk"
				, "ident"		//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
				, "meter"		//	meter number (i.e. 16000913) 4 bytes 
				, "code"		//	metering code - changed at 2019-01-31
				, "maker"		//	manufacturer
				, "tom"			//	time of manufacture
				, "vFirmware"	//	firmwareversion (i.e. 11600000)
				, "vParam"		//	parametrierversion (i.e. 16A098828.pse)
				, "factoryNr"	//	fabrik nummer (i.e. 06441734)
				, "item"		//	ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
				, "mClass"		//	Metrological Class: A, B, C, Q3/Q1, ...
				, "gw"			//	optional gateway pk
				},
				{ cyng::TC_UUID
				, cyng::TC_STRING		//	ident
				, cyng::TC_STRING		//	meter
				, cyng::TC_STRING		//	code
				, cyng::TC_STRING		//	maker
				, cyng::TC_TIME_POINT	//	tom
				, cyng::TC_STRING		//	vFirmware
				, cyng::TC_STRING		//	vParam
				, cyng::TC_STRING		//	factoryNr
				, cyng::TC_STRING		//	item
				, cyng::TC_STRING		//	mClass
				, cyng::TC_UUID			//	gw
				},
				{ 36
				, 24	//	ident
				, 8		//	meter
				, 33	//	country[2], ident[11], number[22]
				, 64	//	maker
				, 0		//	tom
				, 64	//	vFirmware
				, 64	//	vParam
				, 32	//	factoryNr
				, 128	//	item
				, 8		//	mClass 
				, 36	//	gw
				});
		}
		else if (boost::algorithm::equals(name, "TLL")) {

			return cyng::table::make_meta_table<2, 3>(name,
				{ "first"
				, "second"
				, "descr"
				, "enabled"
				, "creationTime" },
				{ cyng::TC_UUID, cyng::TC_UUID, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
				{ 36, 36, 128, 0, 0 });
		}
		else if (boost::algorithm::equals(name, "_Connection")) {

			return cyng::table::make_meta_table<2, 7>(name,
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
				{ 0, 0, 128, 128, 0, 16, 16, 0, 0 });
		}

		else if (boost::algorithm::equals(name, "_Cluster")) {

			return cyng::table::make_meta_table<1, 8>(name, { "tag"	//	client session - primary key [uuid]
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
				{ 36, 0, 32, 0, 0, 0, 0, 0, 0 });
		}
		else if (boost::algorithm::equals(name, "_Config")) {

			return cyng::table::make_meta_table<1, 1>(name, { "name"	//	parameter name
				, "value"	//	parameter value
				},
				{ cyng::TC_STRING, cyng::TC_STRING },
				{ 64, 128 });
		}
		else if (boost::algorithm::equals(name, "_SysMsg")) {

			return cyng::table::make_meta_table<1, 3>(name, { "id"	//	message number
				, "ts"	//	timestamp
				, "severity"
				, "msg"	//	message text
				},
				{ cyng::TC_UINT64, cyng::TC_TIME_POINT, cyng::TC_UINT8, cyng::TC_STRING },
				{ 0, 0, 0, 128 });
		}
		else if (boost::algorithm::equals(name, "_CSV")) {

			return cyng::table::make_meta_table<1, 10>(name,
				{ "tag"			//	[uuid] client session - primary key 
				, "format"		//	[string] SML/IEC
				, "config"		//	[string] local, master, mixed
				, "offset"		//	[u32] minutes after midnight
				, "frame"		//	[u32] time frame in minutes
				, "start15min"	//	[ts] start time of 15 min report
				, "start60min"	//	[ts] start time of 60 min report
				, "start24h"	//	[ts] start time of 24h report
				, "srvCount15min"	//	[size] number of servers with 15 min push
				, "srvCount60min"	//	[size] number of servers with 60 min push
				, "srvCount24h"		//	[size] number of servers with 24 h push
				},
				{ cyng::TC_UUID			//	tag
				, cyng::TC_STRING		//	format
				, cyng::TC_STRING		//	config
				, cyng::TC_MINUTE		//	offset
				, cyng::TC_MINUTE		//	frame
				, cyng::TC_TIME_POINT	//	15minStart
				, cyng::TC_TIME_POINT	//	60minStart
				, cyng::TC_TIME_POINT	//	24hStart
				, cyng::TC_UINT64		//	srvCount15min
				, cyng::TC_UINT64		//	srvCount60min
				, cyng::TC_UINT64		//	srvCount24h
				},
				{ 36, 16, 16, 0, 0, 0, 0, 0, 0 });
		}
		else if (boost::algorithm::equals(name, "TGateway")) {

			return cyng::table::make_meta_table<1, 11>(name, { "pk"	//	primary key
				, "serverId"	//	(1) Server-ID (i.e. 0500153B02517E)
				, "manufacturer"	//	(2) manufacturer (i.e. EMH)
				, "made"		//	(3) production date
				, "factoryNr"	//	(4) fabrik nummer (i.e. 06441734)
				, "ifService"	//	(5) MAC of service interface
				, "ifData"		//	(6) MAC of data interface
				, "pwdDef"		//	(7) Default PW
				, "pwdRoot"		//	(8) root PW
				, "mbus"		//	(9) W-Mbus ID (i.e. A815408943050131)
				, "userName"	//	(10)
				, "userPwd"		//	(11)
				},
				{ cyng::TC_UUID
				, cyng::TC_STRING	//	server ID
				, cyng::TC_STRING	//	manufacturer
				, cyng::TC_TIME_POINT
				, cyng::TC_MAC48
				, cyng::TC_MAC48
				, cyng::TC_STRING
				, cyng::TC_STRING
				, cyng::TC_STRING
				, cyng::TC_STRING
				, cyng::TC_STRING
				},
				{ 36, 23, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 });
		}
		else if (boost::algorithm::equals(name, "_LoRaUplink")) {

			return cyng::table::make_meta_table<1, 10>(name, { "id"	//	message number
				, "ts"	//	timestamp
				, "DevEUI"
				, "FPort"
				, "FCntUp"
				, "ADRbit"
				, "MType"
				, "FCntDn"
				, "CustomerID"
				, "Payload"
				, "tag"
				},
				{ cyng::TC_UINT64		//	id
				, cyng::TC_TIME_POINT	//	ts
				, cyng::TC_MAC64		//	DevEUI
				, cyng::TC_UINT16		//	FPort
				, cyng::TC_INT32		//	FCntUp
				, cyng::TC_INT32		//	ADRbit
				, cyng::TC_INT32		//	MType
				, cyng::TC_INT32		//	FCntDn
				, cyng::TC_STRING		//	CustomerID
				, cyng::TC_STRING		//	Payload
				, cyng::TC_UUID			//	tag
				},
				{ 0		//	id
				, 0		//	ts
				, 19	//	DevEUI
				, 0		//	FPort
				, 0		//	FCntUp
				, 0		//	ADRbit
				, 0		//	MType
				, 0		//	FCntDn
				, 64	//	CustomerID
				, 51	//	Payload
				, 0		//	tag
				});
		}

		//
		//	table name not defined
		//
		return cyng::table::meta_table_ptr();
	}

	bool create_table(cyng::store::db& db, std::string name)
	{
		return db.create_table(create_meta(name));
	}

}