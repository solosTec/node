/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <cyng/store/db.h>
#include <smf/shared/db_schemes.h>

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
			//	(1) [uuid] tag - pk
			//	+-----------------
			//	(1) [string] name - index
			//	+-----------------
			//	(2) name [std::string]
			//	(3) number [std::string]
			//	(4) [std::string] description 
			//	(5) [std::string] identifier (device identifier/type)
			//	(6) [std::string] firmware version 
			//	(7) [bool] enabled (only login allowed)
			//	(8) [std::chrono::system_clock::time_stamp] created 
			//	(9) [std::uint32] query
			return cyng::table::make_meta_table<1, 9, 1>(name,
				{ "pk"
				, "name"
				, "pwd"
				, "msisdn"
				, "descr"
				, "id"	//	model
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

			//
			//	[uuid] device - index
			//
			return cyng::table::make_meta_table<1, 13, 4>(name, { "tag"	//	client session - primary key [uuid]
				, "local"	//	[object] local peer object (hold session reference)
				, "remote"	//	[object] remote peer object (if session connected)
				, "peer"	//	[uuid] remote peer
				, "device"	//	[uuid] - owner of the session
				, "name"	//	[string] - account
				, "source"	//	[uint32] - ipt source id (unique)
				, "loginTime"	//	last login time
				, "rtag"	//	[uuid] remote client session if connected
				, "pLayer"	//	[string] protocol layer (5)
				, "dLayer"	//	[string] data layer (7)
				, "rx"		//	[uint64] received bytes (from device)
				, "sx"		//	[uint64] sent bytes (to device)
				, "px"		//	[uint64] sent push data (to push target)
				},
				{ cyng::TC_UUID		//	tag
				, cyng::TC_STRING	//	local	
				, cyng::TC_STRING	//	remote
				, cyng::TC_UUID		//	peer
				, cyng::TC_UUID		//	device
				, cyng::TC_STRING	//	name
				, cyng::TC_UINT32	//	source
				, cyng::TC_TIME_POINT	//	loginTime
				, cyng::TC_UUID		//	rtag
				, cyng::TC_STRING	//	p-layer
				, cyng::TC_STRING	//	d-layer
				, cyng::TC_UINT64	//	rx
				, cyng::TC_UINT64	//	sx
				, cyng::TC_UINT64 },	//	px
				{ 36, 0, 0, 36, 36, 64, 0, 0, 36, 16, 16, 0, 0, 0 });
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
				{ 0
				, 36	//	tag
				, 36	//	peer
				, 64	//	name
				, 36	//	device
				, 64	//	account
				, 0, 0, 0, 0, 0 });

		}
		else if (boost::algorithm::equals(name, "TMeter")) {

			//
			//	"ident" + "gw" have to be unique
			//
			return cyng::table::make_meta_table<1, 12>(name, 
				{ "pk"
				, "ident"		//	[string] ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
				, "meter"		//	[string] meter number (i.e. 16000913) 4 bytes 
				, "code"		//	[string] metering code - changed at 2019-01-31
				, "maker"		//	[string] manufacturer
				, "tom"			//	[ts] time of manufacture
				, "vFirmware"	//	[string] firmwareversion (i.e. 11600000)
				, "vParam"		//	[string] parametrierversion (i.e. 16A098828.pse)
				, "factoryNr"	//	[string] fabrik nummer (i.e. 06441734)
				, "item"		//	[string] ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
				, "mClass"		//	[string] Metrological Class: A, B, C, Q3/Q1, ...
				, "gw"			//	[uuid] optional gateway pk
				, "protocol"	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
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
				, cyng::TC_STRING		//	protocol
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
				, 32	//	protocol
				});
		}
		else if (boost::algorithm::equals(name, "TMeterAccess")) {

			//
			//	pk is key to table TMeter
			//	ToDo: rename this table to TMeterAccess
			//
			return cyng::table::make_meta_table<1, 5>(name,
				{ "pk"			//	same as TMeter
				, "status"		//	"Statusinformation: 00"
				, "pubKey"	//	Public Key: 18 01 16 05 E6 1E 0D 02 BF 0C FA 35 7D 9E 77 03"
				, "aes"		//	AES-Key: 0102010409060911223344556677AABB
				, "user"
				, "pwd"
				},
				{ cyng::TC_UUID
				, cyng::TC_UINT32		//	status (81 00 60 05 00 00)
				, cyng::TC_BUFFER		//	pubKey
				, cyng::TC_AES128		//	AES 128 (16 bytes)
				, cyng::TC_STRING		//	user
				, cyng::TC_STRING		//	pwd
				},
				{ 36
				, 0		//	status
				, 16	//	pubKey
				, 32	//	aes
				, 32	//	user
				, 32	//	pwd
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

			//
			//	[string] serverId - index
			//
			return cyng::table::make_meta_table<1, 11, 1>(name, 
				{ "pk"			//	primary key
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
				{ 36	//	pk
				, 23	//	serverId
				, 64	//	manufacturer
				, 0		//	made
				, 8		//	factoryNr
				, 18	//	ifService
				, 18	//	ifData
				, 32	//	pwdDef
				, 32	//	pwdRoot	
				, 16	//	mbus
				, 32	//	userName
				, 32	//	userPwd
				});
		}
		else if (boost::algorithm::equals(name, "TGWSnapshot")) {

			return cyng::table::make_meta_table<1, 3>(name, 
				{ "pk"			//	primary key - same as TGateway
				, "gw"			//	gateway ID
				, "remark"		//	short description
				, "lastUpdate"	//	last updated
				},
				{ cyng::TC_UUID
				, cyng::TC_STRING
				, cyng::TC_STRING	//	remark
				, cyng::TC_TIME_POINT
				},
				{ 36	//	pk
				, 23	//	gw
				, 128	//	remark
				, 0		//	lastUpdate
				});
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
		else if (boost::algorithm::equals(name, "_wMBusUplink")) {

			return cyng::table::make_meta_table<1, 7>(name, { "id"	//	message number
				, "ts"			//	timestamp
				, "serverId"	//	example: 01-e61e-13090016-3c-07
				, "medium"
				, "manufacturer"
				, "frameType"
				, "Payload"
				, "tag"
				},
				{ cyng::TC_UINT64		//	id
				, cyng::TC_TIME_POINT	//	ts
				, cyng::TC_STRING		//	serverId
				, cyng::TC_UINT8		//	medium
				, cyng::TC_STRING		//	manufacturer
				, cyng::TC_UINT8		//	frameType
				, cyng::TC_STRING		//	Payload
				, cyng::TC_UUID			//	tag
				},
				{ 0		//	id
				, 0		//	ts
				, 22	//	serverId
				, 0		//	medium	
				, 3		//	manufacturer
				, 0		//	frameType
				, 512	//	Payload
				, 0		//	tag
				});
		}
		else if (boost::algorithm::equals(name, "_IECUplink")) {

			return cyng::table::make_meta_table<1, 4>(name, { "id"	//	message number
				, "ts"			//	timestamp
				, "event"
				, "ep"
				, "tag"
				},
				{ cyng::TC_UINT64		//	id
				, cyng::TC_TIME_POINT	//	ts
				, cyng::TC_STRING		//	Payload
				, cyng::TC_IP_TCP_ENDPOINT	//	ep
				, cyng::TC_UUID			//	tag
				},
				{ 0		//	id
				, 0		//	ts
				, 512	//	event
				, 0		//	ep
				, 0		//	tag
			});
		}
		else if (boost::algorithm::equals(name, "_TimeSeries")) {

			return cyng::table::make_meta_table<1, 5>(name, { "id"	//	message number
				, "ts"	//	timestamp
				, "tag"	//	session tag
				, "account"	//	device/user
				, "evt"	//	event
				, "obj"	//	value
				},
				{ cyng::TC_UINT64
				, cyng::TC_TIME_POINT
				, cyng::TC_UUID			//	tag
				, cyng::TC_STRING		//	account
				, cyng::TC_STRING		//	evt
				, cyng::TC_STRING },	//	obj
				{ 0
				, 0
				, 0			//	tag
				, 64		//	account
				, 64		//	evt
				, 128 });	//	obj
		}
		else if (boost::algorithm::equals(name, "_HTTPSession")) {
			return cyng::table::make_meta_table<1, 6>(name,
				{ "tag"			//	[uuid] client session - primary key 
				, "ep"			//	[ep] remote endpoint
				, "type"		//	[string] HTTP, HTTPS, WS, WSS
				, "start"		//	[ts] start time
				, "authorized"	//	[bool] authorized
				, "user"		//	[string] user name
				, "status"		//	[string] status text
				},
				{ cyng::TC_UUID				//	tag
				, cyng::TC_IP_TCP_ENDPOINT	//	ep
				, cyng::TC_STRING			//	type
				, cyng::TC_TIME_POINT		//	start
				, cyng::TC_BOOL				//	authorized
				, cyng::TC_STRING			//	user
				, cyng::TC_STRING			//	status
				},
				{ 36	//	tag
				, 0		//	ep
				, 6		//	type
				, 0		//	start
				, 0 	//	authorized
				, 64	//	user
				, 128	//	status
			});
		}
		else if (boost::algorithm::equals(name, "TGUIUser")) {

			//
			//	User access rights to dash SPA
			//	[string] name - index
			//	The rights are stored as JSON string e.g. {devices: ["view","edit"], meters:[]}
			//
			return cyng::table::make_meta_table<1, 4, 1>(name, { "pk"
				, "name"		//	[string] login name
				, "pwd"			//	[SHA1] hash of password
				, "lastAccess"	//	[ts] timestamp
				, "privs"		//	[string] JSON
				},
				{ cyng::TC_UUID			//	pk
				, cyng::TC_STRING		//	name
				, cyng::TC_DIGEST_SHA1	//	pwd
				, cyng::TC_TIME_POINT	//	lastAccess
				, cyng::TC_STRING		//	rights
				},
				{ 36
				, 32	//	name
				, 40	//	pwd
				, 0		//	lastAccess
				, 2048	//	rights
				});
		}
		else if (boost::algorithm::equals(name, "TNodeNames")) {

			//
			//	This tables stores a relation between a node UUID
			//	and an optional name
			//
			return cyng::table::make_meta_table<1, 1>(name, { "pk"
				, "name"		//	[string] module name
				},
				{ cyng::TC_UUID			//	pk
				, cyng::TC_STRING		//	name
				},
				{ 36
				, 64	//	name
				});
		}
		else if (boost::algorithm::equals(name, "TBridge")) {

			//
			//	This table contains addition information about 
			//	meters with IEC communiction.
			//
			return cyng::table::make_meta_table<1, 4>(name, 
				{ "pk"		//	same key as in TMeter table
				, "address"		//	[ip] incoming/outgoing IP connection
				, "port"		//	[ip] incoming/outgoing IP connection
				, "direction"	//	[bool] true = incoming, false = outgoing
				, "interval"	//	[seconds] pull cycle
				},
				{ cyng::TC_UUID			//	pk
				, cyng::TC_IP_ADDRESS	//	host address
				, cyng::TC_UINT16		//	TCP/IP port
				, cyng::TC_BOOL
				, cyng::TC_SECOND
				},
				{ 36
				, 0	//	address
				, 0	//	port
				, 0	//	direction
				, 0
				});
		}

		else if (boost::algorithm::equals(name, "TLocation")) {

			//
			//	This table contains location data of meters
			//
			return cyng::table::make_meta_table<1, 7>(name,
				{ "pk"		//	same key as in TMeter table
				, "desc"		//	Description
				, "country"		//	street, city
				, "region"		//	general location
				, "address"		//	street, city
				, "lat"			//	latitude
				, "long"		//	longitude
				, "projection"	//	mapping format
				},
				{ cyng::TC_UUID		//	pk
				, cyng::TC_STRING	//	desc
				, cyng::TC_STRING	//	country
				, cyng::TC_STRING	//	region
				, cyng::TC_STRING	//	address
				, cyng::TC_DOUBLE	//	lat
				, cyng::TC_DOUBLE	//	long
				, cyng::TC_STRING	//	projection
				},
				{ 36
				, 128	//	desc
				, 32	//	country
				, 32	//	region
				, 64	//	address
				, 0		//	lat
				, 0		//	long
				, 16	//	projection
			});
		}
		else if (boost::algorithm::equals(name, "_Broker")) {
			//
			//	Define a list of all servers to listen for incoming raw wireless
			//	M-Bus data
			//	Note: This table is currently only used by meter broker, but data
			//	are not used anywhere. Therefore this table should be removed.
			//
			return cyng::table::make_meta_table<1, 3>(name,
				{ "pk"			//	[uuid] tag from node (cluster session tag)
				, "address"		//	[ip] incoming/outgoing IP connection
				, "port"		//	[u16] incoming/outgoing IP connection
				, "protocol"	//	[string] M-Bus, IEC, ...
				},
				{ cyng::TC_UUID			//	pk
				, cyng::TC_IP_ADDRESS	//	host address
				, cyng::TC_UINT16		//	TCP/IP port
				, cyng::TC_STRING
				},
				{ 36
				, 0		//	address
				, 0		//	port
				, 32	//	protocol
				});
		}
		else if (boost::algorithm::equals(name, "TCSVReport")) {

			//
			//	Configuration data for CSV reports
			//
			return cyng::table::make_meta_table<1, 4>(name,
				{ "profile"		//	profile name (OBIS code)
				, "rootDir"		//	[string] root directory
				, "prefix"		//	[string] 
				, "suffix"		//	[string] 
				, "header"		//	[bool] 
				},
				{ cyng::TC_STRING	//	profile
				, cyng::TC_STRING	//	rootDir
				, cyng::TC_STRING	//	prefix
				, cyng::TC_STRING	//	suffix
				, cyng::TC_BOOL		//	header
				},
				{ 12
				, 0	//	rootDir
				, 0	//	prefix
				, 0	//	suffix
				, 0	//	header
				});
		}

		//	Configurable Validation, Estimation and Editing (VEE) rules provide maximum flexibility
		else if (boost::algorithm::equals(name, "TValidationReport")) {

			//
			//	Contains data of validation report
			//
			return cyng::table::make_meta_table<1, 5>(name,
				{ "pk"			//	[string] 
				, "from"		//	[ts] start time
				, "to"			//	[ts] end time
				, "type"		//	[u32] type of validation (missing, implausible)
				, "quantity"	//	[u32] number of missing records
				, "edited"		//	[bool]
				},
				{ cyng::TC_STRING	//	profile
				, cyng::TC_TIME_POINT	//	from
				, cyng::TC_TIME_POINT	//	to
				, cyng::TC_UINT32	//	type
				, cyng::TC_UINT32	//	quantity
				, cyng::TC_BOOL		//	edited
				},
				{ 36
				, 0	//	from
				, 0	//	to
				, 0	//	type
				, 0	//	quantity
				, 0	//	edited
				});
		}
		else if (boost::algorithm::equals(name, "_EventQueue")) {

			//
			//	Contains data of validation report
			//
			return cyng::table::make_meta_table<1, 5>(name,
				{ "pk"			//	[uuid] same key as TMeter
				, "table"		//	[string] table name
				, "key"			//	[vec] any vector as table key
				, "at"			//	[ts] start time
				, "type"		//	[string] event type
				, "data"		//	[pm]
				},
				{ cyng::TC_UUID		//	pk
				, cyng::TC_STRING	//	table
				, cyng::TC_VECTOR	//	key
				, cyng::TC_TIME_POINT	//	at
				, cyng::TC_STRING		//	type
				, cyng::TC_PARAM_MAP	//	data
				},
				{ 36
				, 0		//	table
				, 0		//	key
				, 0		//	at
				, 0		//	type
				, 0		//	data
				});
		}
		
		//
		//	table name not defined
		//
		return cyng::table::meta_table_ptr();
	}

	bool create_table(cyng::store::db& db
		, std::string const& table_name
		, bool pass_through)
	{
		return db.create_table(create_meta(table_name), pass_through);
	}

}