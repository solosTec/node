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
		//cyng::meta_sql get_table_cluster() {
		//	return cyng::to_sql(get_store_cluster(), { 36, 0, 32, 0, 0, 0, 0, 0 });
		//}

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
					cyng::column("creationTime", cyng::TC_TIME_POINT),
					cyng::column("query", cyng::TC_UINT32)
				}
			, 1);

		}

		cyng::meta_sql get_table_device() {
			return cyng::to_sql(get_store_device(), { 36, 128, 32, 64, 512, 64, 64, 0, 0, 0 });
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

		cyng::meta_store get_store_target() {

			return cyng::meta_store("target"
				, {
					cyng::column("channel", cyng::TC_UINT32),
					cyng::column("tag", cyng::TC_UUID),			//	owner session
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

	}
}

