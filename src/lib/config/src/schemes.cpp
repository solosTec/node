/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/config/schemes.h>
#include <smf.h>

#include <boost/assert.hpp>

namespace smf {
	namespace config {

		cyng::meta_store get_store_cluster() {

			return cyng::meta_store("cluster"
				, {
					cyng::column("tag", cyng::TC_UUID),
					cyng::column("class", cyng::TC_STRING),
					cyng::column("loginTime", cyng::TC_TIME_POINT),	//	last login time
					cyng::column("version", cyng::TC_VERSION),		//	>= 0.9
					cyng::column("clients", cyng::TC_UINT64),		//	clients
					cyng::column("ping", cyng::TC_MICRO_SECOND),	//	ping time
					cyng::column("ep", cyng::TC_IP_TCP_ENDPOINT),	//	seen from main node
					cyng::column("pid", cyng::TC_PID)				//	local process ID
				}
			, 1);
		}

		cyng::meta_store get_store_device() {

			return cyng::meta_store("device"
				, {
					cyng::column("tag", cyng::TC_UUID),
					cyng::column("name", cyng::TC_STRING),
					cyng::column("pwd", cyng::TC_STRING),	
					cyng::column("msisdn", cyng::TC_STRING),
					cyng::column("descr", cyng::TC_STRING),
					cyng::column("id", cyng::TC_STRING),	//	model
					cyng::column("vFirmware", cyng::TC_STRING),
					cyng::column("enabled", cyng::TC_BOOL),
					cyng::column("creationTime", cyng::TC_TIME_POINT)
				}
			, 1);

		}

		cyng::meta_sql get_table_device() {
			return cyng::to_sql(get_store_device(), { 36, 128, 32, 64, 512, 64, 64, 0, 0 });
		}

		cyng::meta_store get_store_meter() {
			return cyng::meta_store("meter"
				, {
					cyng::column("tag", cyng::TC_UUID),			//	same as device
					cyng::column("ident", cyng::TC_STRING),		//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
					cyng::column("meter", cyng::TC_STRING),		//	[string] meter number (i.e. 16000913) 4 bytes 
					cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at 2019-01-31
					cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
					cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of manufacture
					cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
					cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
					cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
					cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
					cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
					cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
					cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
				}
			, 1);

		}
		cyng::meta_sql get_table_meter() {
			return cyng::to_sql(get_store_meter(), { 36
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

		cyng::meta_store get_store_meterIEC() {
			return cyng::meta_store("meterIEC"
				, {
					cyng::column("tag", cyng::TC_UUID),			//	same as meter/device
					cyng::column("host", cyng::TC_STRING),		//	host/domain name
					cyng::column("port", cyng::TC_UINT16),		//	listener port (outgoing)
					cyng::column("interval", cyng::TC_SECOND)	//	pull cycle
				}
			, 1);

		}
		cyng::meta_sql get_table_meterIEC() {
			return cyng::to_sql(get_store_meterIEC(), { 36
				, 128	//	address
				, 0		//	port
				, 0
				});

		}

		cyng::meta_store get_store_meterwMBus() {
			return cyng::meta_store("meterwMBus"
				, {
					cyng::column("tag", cyng::TC_UUID),				//	same as meter/device
					cyng::column("address", cyng::TC_IP_ADDRESS),	//	listener address
					cyng::column("port", cyng::TC_UINT16),			//	listener port (incoming)
					cyng::column("aes", cyng::TC_AES128)		//	AES 128 key
				}
			, 1);

		}
		cyng::meta_sql get_table_meterwMBus() {
			return cyng::to_sql(get_store_meterwMBus(), { 36
				, 0	//	address
				, 0	//	port
				, 32
				});
		}

		cyng::meta_store get_store_gateway() {
			return cyng::meta_store("gateway"
				, {
					cyng::column("tag", cyng::TC_UUID),				//	same as device
					cyng::column("serverId", cyng::TC_STRING),		//	(1) Server-ID (i.e. 0500153B02517E)
					cyng::column("manufacturer", cyng::TC_STRING),	//	(2) manufacturer (i.e. EMH)
					cyng::column("tom", cyng::TC_TIME_POINT),		//	(3) production date
					cyng::column("factoryNr", cyng::TC_STRING),		//	(4) fabrik nummer (i.e. 06441734)
					cyng::column("ifService", cyng::TC_MAC48),		//	(5) MAC of service interface
					cyng::column("ifData", cyng::TC_MAC48),		//	(6) MAC of WAN interface
					//cyng::column("ifHAN", cyng::TC_STRING),		//	(6) MAC of HAN interface
					cyng::column("pwdDef", cyng::TC_STRING),		//	(7) Default PW
					cyng::column("pwdRoot", cyng::TC_STRING),		//	(8) root PW
					cyng::column("mbus", cyng::TC_STRING),			//	(9) W-Mbus ID (i.e. A815408943050131)
					cyng::column("userName", cyng::TC_STRING),		//	(10)
					cyng::column("userPwd", cyng::TC_STRING)		//	(11)
				}
			, 1);

		}
		cyng::meta_sql get_table_gateway() {
			return cyng::to_sql(get_store_gateway(), { 36	//	pk
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

		cyng::meta_store get_store_lora() {
			return cyng::meta_store("loRaDevice"
				, {
					cyng::column("tag", cyng::TC_UUID),
					cyng::column("DevEUI", cyng::TC_MAC64),
					cyng::column("AESKey", cyng::TC_AES256),	// 256 Bit
					cyng::column("driver", cyng::TC_STRING),
					cyng::column("activation", cyng::TC_BOOL),	//	OTAA/ABP
					cyng::column("DevAddr", cyng::TC_UINT32),	//	32 bit device address (non-unique)
					cyng::column("AppEUI", cyng::TC_MAC64),		//	64 bit application identifier, EUI-64 (unique)
					cyng::column("GatewayEUI", cyng::TC_MAC64)	//	64 bit gateway identifier, EUI-64 (unique)
				}
			, 1);

		}
		cyng::meta_sql get_table_lora() {
			return cyng::to_sql(get_store_lora(), { 36, 0, 64, 32, 0, 0, 0, 0 });
		}

		cyng::meta_store get_store_gui_user() {
			return cyng::meta_store("guiUser"
				, {
					cyng::column("tag", cyng::TC_UUID),
					cyng::column("name", cyng::TC_STRING),
					cyng::column("pwd", cyng::TC_DIGEST_SHA1),	// SHA1 hash
					cyng::column("lastAccess", cyng::TC_TIME_POINT),
					cyng::column("rights", cyng::TC_STRING)	//	access rights (JSON)
				}
			, 1);

		}
		cyng::meta_sql get_table_gui_user() {
			return cyng::to_sql(get_store_gui_user(), { 36, 32, 40, 0, 4096 });
		}

		cyng::meta_store get_store_location() {
			return cyng::meta_store("location"
				, {
					cyng::column("tag", cyng::TC_UUID),			//	same key as in TMeter table
					cyng::column("desc", cyng::TC_STRING),		//	Description
					cyng::column("country", cyng::TC_STRING),	//	street, city
					cyng::column("region", cyng::TC_STRING),	//	general location
					cyng::column("address", cyng::TC_STRING),	//	street, city
					cyng::column("lat", cyng::TC_DOUBLE),		//	latitude
					cyng::column("long", cyng::TC_DOUBLE),		//	longitude
					cyng::column("projection", cyng::TC_STRING)	//	mapping format
				}
			, 1);
		}
		cyng::meta_sql get_table_location() {
			return cyng::to_sql(get_store_location(), { 36
				, 128	//	desc
				, 32	//	country
				, 32	//	region
				, 64	//	address
				, 0		//	lat
				, 0		//	long
				, 16	//	projection
				});
		}

		cyng::meta_store get_store_session() {

			return cyng::meta_store("session"
				, {
					cyng::column("tag", cyng::TC_UUID),			//	pty 
					cyng::column("peer", cyng::TC_UUID),		//	peer of owner
					cyng::column("name", cyng::TC_STRING),		//	account
					cyng::column("ep", cyng::TC_IP_TCP_ENDPOINT),	//	seen from pty
					cyng::column("device", cyng::TC_UUID),		//	owner of target
					cyng::column("source", cyng::TC_UINT32),	//	ip-t source id (unique)
					cyng::column("loginTime", cyng::TC_TIME_POINT),	//	login time
					cyng::column("rtag", cyng::TC_UUID),		//	remote client session if connected
					cyng::column("pLayer", cyng::TC_STRING),	//	protocol layer (from session)
					cyng::column("dLayer", cyng::TC_STRING),	//	data layer (from pty)
					cyng::column("rx", cyng::TC_UINT64),		//	received bytes (from device)
					cyng::column("sx", cyng::TC_UINT64),		//	sent bytes (to device)
					cyng::column("px", cyng::TC_UINT64)			//	sent push data (to push target)
				}
			, 1);
		}

		cyng::meta_store get_store_connection() {
			return cyng::meta_store("connection"
				, {
					cyng::column("first", cyng::TC_UUID),		//	primary key 
					cyng::column("second", cyng::TC_UUID),		//	primary key 
					cyng::column("caller", cyng::TC_STRING),	//	account
					cyng::column("callee", cyng::TC_STRING),	//	seen from pty
					cyng::column("start", cyng::TC_IP_TCP_ENDPOINT),	//	owner of target
					cyng::column("local", cyng::TC_BOOL),	//	true if local connection
					cyng::column("throughput", cyng::TC_UINT64)			//	data throughput
				}
			, 2);
		}

		cyng::meta_store get_store_target() {

			return cyng::meta_store("target"
				, {
					cyng::column("tag", cyng::TC_UINT32),		//	IP-T channel
					cyng::column("tagSession", cyng::TC_UUID),	//	owner session
					cyng::column("peer", cyng::TC_UUID),		//	peer of owner
					cyng::column("name", cyng::TC_STRING),		//	target id
					cyng::column("device", cyng::TC_UUID),		//	owner of target
					cyng::column("account", cyng::TC_STRING),	//	name of target owner
					cyng::column("pSize", cyng::TC_UINT16),		//	packet size
					cyng::column("wSize", cyng::TC_UINT8),		//	window size
					cyng::column("regTime", cyng::TC_TIME_POINT),	//	registration time
					cyng::column("px", cyng::TC_UINT64),		//	incoming data in bytes
					cyng::column("counter", cyng::TC_UINT64)	//	message counter
				}
			, 1);
		}

		cyng::meta_store get_config() {
			return cyng::meta_store("config"
				, {
					cyng::column("key", cyng::TC_STRING),	//	key
					cyng::column("value", cyng::TC_NULL)	//	any data type allowed	
				}
			, 1);

		}

		cyng::meta_store get_store_sys_msg() {
			return cyng::meta_store("sysMsg"
				, {
					cyng::column("tag", cyng::TC_UINT64),			//	message number
					cyng::column("ts", cyng::TC_TIME_POINT),		//	timestamp
					cyng::column("severity", cyng::TC_SEVERITY),
					cyng::column("msg", cyng::TC_STRING)			//	message text
				}
			, 1);
		}


		cyng::meta_store get_store_uplink_lora() {
			return cyng::meta_store("loRaUplink"
				, {
					cyng::column("tag", cyng::TC_UINT64),		//	message number
					cyng::column("ts", cyng::TC_TIME_POINT),	//	timestamp
					cyng::column("DevEUI", cyng::TC_MAC64),
					cyng::column("FPort", cyng::TC_UINT16),
					cyng::column("FCntUp", cyng::TC_INT32),
					cyng::column("ADRbit", cyng::TC_INT32),
					cyng::column("MType", cyng::TC_INT32),
					cyng::column("FCntDn", cyng::TC_INT32),
					cyng::column("CustomerID", cyng::TC_STRING),
					cyng::column("Payload", cyng::TC_STRING),
					cyng::column("tagLora", cyng::TC_UUID)
				}
			, 1);
		}
		cyng::meta_store get_store_uplink_iec() {
			return cyng::meta_store("iecUplink"
				, {
					cyng::column("tag", cyng::TC_UINT64),		//	message number
					cyng::column("ts", cyng::TC_TIME_POINT),	//	timestamp
					cyng::column("event", cyng::TC_STRING),		//	description
					cyng::column("ep", cyng::TC_UINT8),
					cyng::column("tagSession", cyng::TC_UUID)
				}
			, 1);

		}
		cyng::meta_store get_store_uplink_wmbus() {
			return cyng::meta_store("wMBusUplink"
				, {
					cyng::column("tag", cyng::TC_UINT64),		//	message number
					cyng::column("ts", cyng::TC_TIME_POINT),	//	timestamp
					cyng::column("serverId", cyng::TC_STRING),	//	example: 01-e61e-13090016-3c-07
					cyng::column("medium", cyng::TC_UINT8),
					cyng::column("manufacturer", cyng::TC_STRING),
					cyng::column("frameType", cyng::TC_UINT8),
					cyng::column("payload", cyng::TC_STRING),
					cyng::column("tagSession", cyng::TC_UUID)
				}
			, 1);

		}

	}
}

