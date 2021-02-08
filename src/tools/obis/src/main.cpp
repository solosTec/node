/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/defs.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/tag.hpp>
#include <iostream>
#include <map>
#include <fstream>
#include <iomanip>

 /**
  * internal class to describe an OBIS code
  */
class obis_ctx {
public:
	obis_ctx(char const* name, std::size_t type, char const* desc)	
		: name_(name)
		, type_(type)
		, desc_(desc)
	{}
	std::string get_name() const {
		return name_;
	}
	std::string get_desc() const {
		return desc_;
	}
	std::size_t get_type() const {
		return type_;
	}
private:
	char const* name_;
	std::size_t const type_;
	char const* desc_;
};


 /**
  * Use this program to verify if there are duplicates in the OBIS db
  */
int main(int argc, char** argv) {

	using obis_map_t = std::map< cyng::obis, obis_ctx >;

	static obis_map_t obis_map {

		{ DEFINE_OBIS(00, 00, 00, 00, 00, FF), {"METER_ADDRESS", cyng::TC_NULL, ""}},
		{ DEFINE_OBIS(00, 00, 00, 00, 01, FF), {"IDENTITY_NR_1", cyng::TC_NULL, ""}},
		{ DEFINE_OBIS(00, 00, 00, 00, 02, FF), {"IDENTITY_NR_2", cyng::TC_NULL, ""}},
		{ DEFINE_OBIS(00, 00, 00, 00, 03, FF), {"IDENTITY_NR_3", cyng::TC_NULL, ""}},
		{ DEFINE_OBIS(00, 00, 00, 01, 00, FF), {"RESET_COUNTER", cyng::TC_NULL, ""}},

		{ DEFINE_OBIS(00, 00, 00, 02, 00, ff), {"FIRMWARE_VERSION", cyng::TC_NULL, "COSEM class id 1"}},

		{ DEFINE_OBIS(00, 00, 01, 00, 00, ff), {"REAL_TIME_CLOCK", cyng::TC_NULL, "current time"}},

		{ DEFINE_OBIS(00, 00, 60, 01, 00, ff), {"SERIAL_NR", cyng::TC_NULL, "(C.1.0) Serial number I (assigned by the manufacturer)"}},
		{ DEFINE_OBIS(00, 00, 60, 01, 01, ff), {"SERIAL_NR_SECOND", cyng::TC_NULL, "Serial number II (assigned by the manufacturer)."}},
		{ DEFINE_OBIS(00, 00, 60, 01, 03, ff), {"PRODUCTION_DATE", cyng::TC_NULL, "(C.1.3) date of manufacture"}},
		{ DEFINE_OBIS(00, 00, 60, 01, ff, ff), {"FABRICATION_NR", cyng::TC_NULL, "fabrication number"}},
		{ DEFINE_OBIS(00, 00, 60, 02, 01, ff), {"DATE_TIME_PARAMETERISATION", cyng::TC_UINT32, ""}},
		{ DEFINE_OBIS(00, 00, 60, 07, 00, ff), {"POWER_OUTAGES", cyng::TC_NULL, "Number of power failures "}},
		{ DEFINE_OBIS(00, 00, 60, 08, 00, ff), {"SECONDS_INDEX", cyng::TC_NULL, "[SML_Time] seconds index"}},
		{ smf::OBIS_LOGICAL_NAME, {"LOGICAL_NAME", cyng::TC_NULL, ""}},
		{ smf::OBIS_HARDWARE_TYPE, {"HARDWARE_TYPE", cyng::TC_STRING, "name"}},
		{ smf::OBIS_MBUS_STATE, {"MBUS_STATE", cyng::TC_NULL, "Status according to EN13757-3 (error register)"}},
		{ smf::OBIS_MBUS_STATE_1, {"MBUS_STATE_1", cyng::TC_NULL, "not relevant under calibration law"}},
		{ smf::OBIS_MBUS_STATE_2, {"MBUS_STATE_2", cyng::TC_NULL, "not relevant under calibration law"}},
		{ smf::OBIS_MBUS_STATE_3, {"MBUS_STATE_3", cyng::TC_NULL, "not relevant under calibration law"}},

		{ smf::OBIS_STORAGE_TIME_SHIFT, {"STORAGE_TIME_SHIFT", cyng::TC_NULL, "root push operations"}},
		{ smf::OBIS_HAS_SSL_CONFIG, {"HAS_SSL_CONFIG", cyng::TC_NULL, "SSL/TSL configuration available"}},
		{ smf::OBIS_SSL_CERTIFICATES, {"SSL_CERTIFICATES", cyng::TC_NULL, "list of SSL certificates"}},

		{ smf::OBIS_ROOT_SECURITY, {"ROOT_SECURITY", cyng::TC_NULL, "list some basic security information"}},
		{ smf::OBIS_SECURITY_SERVER_ID, {"SECURITY_SERVER_ID", cyng::TC_NULL, ""}},
		{ smf::OBIS_SECURITY_OWNER, {"SECURITY_OWNER", cyng::TC_NULL, ""}},
		{ smf::OBIS_SECURITY_05, {"SECURITY_05", cyng::TC_NULL, "3"}},
		{ smf::OBIS_SECURITY_06, {"SECURITY_06", cyng::TC_NULL, "3"}},
		{ smf::OBIS_SECURITY_06_10, {"SECURITY_06_10", cyng::TC_NULL, "3"}},
		{ smf::OBIS_SECURITY_07, {"SECURITY_07", cyng::TC_NULL, "1"}},

		{ smf::OBIS_CLASS_OP_LSM_STATUS, {"CLASS_OP_LSM_STATUS", cyng::TC_NULL, "LSM status"}},
		{ smf::OBIS_ACTUATORS, {"ACTUATORS", cyng::TC_NULL, "list of actuators"}},
		{ smf::OBIS_CLASS_OP_LSM_ACTOR_ID, {"CLASS_OP_LSM_ACTOR_ID", cyng::TC_NULL, "ServerID des Aktors (uint16)"}},
		{ smf::OBIS_CLASS_OP_LSM_CONNECT, {"CLASS_OP_LSM_CONNECT", cyng::TC_NULL, "Connection state"}},
		{ smf::OBIS_CLASS_OP_LSM_SWITCH, {"CLASS_OP_LSM_SWITCH", cyng::TC_NULL, "Schaltanforderung"}},
		{ smf::OBIS_CLASS_OP_LSM_FEEDBACK, {"CLASS_OP_LSM_FEEDBACK", cyng::TC_NULL, "feedback configuration"}},
		{ smf::OBIS_CLASS_OP_LSM_LOAD, {"CLASS_OP_LSM_LOAD", cyng::TC_NULL, "number of loads"}},
		{ smf::OBIS_CLASS_OP_LSM_POWER, {"CLASS_OP_LSM_POWER", cyng::TC_NULL, "total power"}},
		{ smf::OBIS_CLASS_STATUS, {"CLASS_STATUS", cyng::TC_NULL, "see: 2.2.1.3 Status der Aktoren (Kontakte)"}},
		{ smf::OBIS_CLASS_OP_LSM_VERSION, {"CLASS_OP_LSM_VERSION", cyng::TC_NULL, "LSM version"}},
		{ smf::OBIS_CLASS_OP_LSM_TYPE, {"CLASS_OP_LSM_TYPE", cyng::TC_NULL, "LSM type"}},
		{ smf::OBIS_CLASS_OP_LSM_ACTIVE_RULESET, {"CLASS_OP_LSM_ACTIVE_RULESET", cyng::TC_NULL, "active ruleset"}},
		//{ smf::OBIS_CLASS_RADIO_KEY, {"CLASS_RADIO_KEY", cyng::TC_NULL, "or radio key"}},
		{ smf::OBIS_CLASS_OP_LSM_PASSIVE_RULESET, {"CLASS_OP_LSM_PASSIVE_RULESET", cyng::TC_NULL, ""}},
		{ smf::OBIS_CLASS_OP_LSM_JOB, {"CLASS_OP_LSM_JOB", cyng::TC_NULL, "Schaltauftrags ID ((octet string)	"}},
		{ smf::OBIS_CLASS_OP_LSM_POSITION, {"CLASS_OP_LSM_POSITION", cyng::TC_NULL, "current position"}},

		{ smf::OBIS_CLASS_MBUS, {"CLASS_MBUS", cyng::TC_NULL, ""}},
		{ smf::OBIS_CLASS_MBUS_RO_INTERVAL, {"CLASS_MBUS_RO_INTERVAL", cyng::TC_NULL, "readout interval in seconds % 3600"}},
		{ smf::OBIS_CLASS_MBUS_SEARCH_INTERVAL, {"CLASS_MBUS_SEARCH_INTERVAL", cyng::TC_NULL, "search interval in seconds"}},
		{ smf::OBIS_CLASS_MBUS_SEARCH_DEVICE, {"CLASS_MBUS_SEARCH_DEVICE", cyng::TC_BOOL, "search device now and by restart"}},
		{ smf::OBIS_CLASS_MBUS_AUTO_ACTIVATE, {"CLASS_MBUS_AUTO_ACTIVATE", cyng::TC_BOOL, "automatic activation of meters"}},
		{ smf::OBIS_CLASS_MBUS_BITRATE, {"CLASS_MBUS_BITRATE", cyng::TC_NULL, "used baud rates(bitmap)"}},

		//	
		{ smf::OBIS_SERVER_ID_1_1, {"SERVER_ID_1_1", cyng::TC_NULL, "Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)"}},
		{ smf::OBIS_SERVER_ID_1_2, {"SERVER_ID_1_2", cyng::TC_NULL, "Identifikationsnummer 1.2"}},
		{ smf::OBIS_SERVER_ID_1_3, {"SERVER_ID_1_3", cyng::TC_NULL, "Identifikationsnummer 1.3"}},
		{ smf::OBIS_SERVER_ID_1_4, {"SERVER_ID_1_4", cyng::TC_NULL, "Identifikationsnummer 1.4"}},
		{ smf::OBIS_DEVICE_ID, {"DEVICE_ID", cyng::TC_NULL, "encode profiles"}},
		{ smf::OBIS_SOFTWARE_ID, {"SOFTWARE_ID", cyng::TC_BUFFER, ""}},

		{ smf::OBIS_CURRENT_UTC, {"CURRENT_UTC", cyng::TC_NULL, "readout time in UTC"}},

		{ smf::OBIS_TCP_WAIT_TO_RECONNECT, {"TCP_WAIT_TO_RECONNECT", cyng::TC_UINT8, ""}},
		{ smf::OBIS_TCP_CONNECT_RETRIES, {"TCP_CONNECT_RETRIES", cyng::TC_UINT32, ""}},

		{ smf::OBIS_PUSH_SERVICE, {"PUSH_SERVICE", cyng::TC_NULL, "options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX"}},


		/**
		 *
		 */
		{ smf::OBIS_OBISLOG_INTERVAL, {"OBISLOG_INTERVAL", cyng::TC_UINT16, " Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook"}},

		{ smf::OBIS_PUSH_SERVICE_IPT, {"PUSH_SERVICE_IPT", cyng::TC_NULL, "SML data response without request - typical IP - T push"}},
		{ smf::OBIS_PUSH_SERVICE_SML, {"PUSH_SERVICE_SML", cyng::TC_NULL, "SML data response without request - target is SML client address"}},
		//{ smf::OBIS_PUSH_SERVICE_KNX, {"PUSH_SERVICE_KNX", cyng::TC_NULL, ""}}, //	target is KNX ID
		{ smf::OBIS_DATA_COLLECTOR_REGISTER, {"DATA_COLLECTOR_REGISTER", cyng::TC_NULL, "OBIS list (data mirror)"}},


		//
		//	sensor/meter parameters
		//

		{ smf::OBIS_DATA_USER_NAME, {"DATA_USER_NAME", cyng::TC_NULL, ""}},
		{ smf::OBIS_DATA_USER_PWD, {"DATA_USER_PWD", cyng::TC_NULL, ""}},


		{ smf::OBIS_CLEAR_DATA_COLLECTOR, {"CLEAR_DATA_COLLECTOR", cyng::TC_NULL, ""}},
		{ smf::OBIS_SET_ACTIVATE_FW, {"SET_ACTIVATE_FW", cyng::TC_NULL, ""}},

		{ smf::OBIS_DATA_AES_KEY, {"DATA_AES_KEY", cyng::TC_NULL, ""}},

		//
		//	Profiles (Lastgänge)
		//	The OBIS code to encode profiles is 81 81 C7 8A 83 FF
		//

		{ smf::OBIS_PROFILE_1_MINUTE, {"PROFILE_1_MINUTE", cyng::TC_NULL, "1 minute"}},	//	
		{ smf::OBIS_PROFILE_15_MINUTE, {"PROFILE_15_MINUTE", cyng::TC_NULL, ""}},
		{ smf::OBIS_PROFILE_60_MINUTE, {"PROFILE_60_MINUTE", cyng::TC_NULL, ""}},
		{ smf::OBIS_PROFILE_24_HOUR, {"PROFILE_24_HOUR", cyng::TC_NULL, ""}},
		{ smf::OBIS_PROFILE_LAST_2_HOURS, {"PROFILE_LAST_2_HOURS", cyng::TC_NULL, "past two hours"}},	//	
		{ smf::OBIS_PROFILE_LAST_WEEK, {"PROFILE_LAST_WEEK", cyng::TC_NULL, "weekly (on day change from Sunday to Monday)"}},	//	
		{ smf::OBIS_PROFILE_1_MONTH, {"PROFILE_1_MONTH", cyng::TC_NULL, "monthly recorded meter readings"}},	//	
		{ smf::OBIS_PROFILE_1_YEAR, {"PROFILE_1_YEAR", cyng::TC_NULL, "annually recorded meter readings"}},	//	
		{ smf::OBIS_PROFILE_INITIAL, {"PROFILE_INITIAL", cyng::TC_NULL, "81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods"}},	//	

		{ smf::OBIS_ROOT_PUSH_OPERATIONS, {"ROOT_PUSH_OPERATIONS", cyng::TC_NULL, "push root element"}},	//	 

		{ smf::OBIS_PUSH_INTERVAL, {"PUSH_INTERVAL", cyng::TC_NULL, "in seconds"}},	//	
		{ smf::OBIS_PUSH_DELAY, {"PUSH_DELAY", cyng::TC_NULL, "in seconds"}},	//	

		{ smf::OBIS_PUSH_SOURCE, {"PUSH_SOURCE", cyng::TC_NULL, "options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST"}},	//	
		{ smf::OBIS_PUSH_SOURCE_PROFILE, {"PUSH_SOURCE_PROFILE", cyng::TC_NULL, "new meter/sensor data"}},	//!< 
		{ smf::OBIS_PUSH_SOURCE_INSTALL, {"PUSH_SOURCE_INSTALL", cyng::TC_NULL, "configuration changed"}},	//!< 
		{ smf::OBIS_PUSH_SOURCE_VISIBLE_SENSORS, {"PUSH_SOURCE_VISIBLE_SENSORS", cyng::TC_NULL, "list of visible meters changed"}},	//!< 
		{ smf::OBIS_PUSH_SOURCE_ACTIVE_SENSORS, {"PUSH_SOURCE_ACTIVE_SENSORS", cyng::TC_NULL, "list of active meters changed"}},	//!< 

		{ smf::OBIS_PUSH_SERVER_ID, {"PUSH_SERVER_ID", cyng::TC_NULL, ""}},
		{ smf::OBIS_PUSH_IDENTIFIERS, {"PUSH_IDENTIFIERS", cyng::TC_NULL, "list of identifiers of the values to be delivered by the push source"}},	//	
		{ smf::OBIS_PROFILE, {"PROFILE", cyng::TC_NULL, "encode profiles"}},	//	

		//
		//	root elements
		//81, 81, C7, 81, 0E, FF 
		{ smf::OBIS_ROOT_MEMORY_USAGE, {"ROOT_MEMORY_USAGE", cyng::TC_NULL, "request memory usage"}},	//	
		{ smf::OBIS_ROOT_MEMORY_MIRROR, {"ROOT_MEMORY_MIRROR", cyng::TC_NULL, ""}},
		{ smf::OBIS_ROOT_MEMORY_TMP, {"ROOT_MEMORY_TMP", cyng::TC_NULL, ""}},

		{ smf::OBIS_ROOT_CUSTOM_INTERFACE, {"ROOT_CUSTOM_INTERFACE", cyng::TC_NULL, "see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle"}},	//	

		{ smf::OBIS_CUSTOM_IF_IP_REF, {"CUSTOM_IF_IP_REF", cyng::TC_UINT8, "0 == manual, 1 == DHCP"}},	//	[u8] 
		{ smf::OBIS_CUSTOM_IF_IP_ADDRESS_1, {"CUSTOM_IF_IP_ADDRESS_1", cyng::TC_IP_ADDRESS, "first manual set IP address"}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_IP_ADDRESS_2, {"CUSTOM_IF_IP_ADDRESS_2", cyng::TC_IP_ADDRESS, "second manual set IP address"}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_IP_MASK_1, {"CUSTOM_IF_IP_MASK_1", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_IP_MASK_2, {"CUSTOM_IF_IP_MASK_2", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_DHCP, {"CUSTOM_IF_DHCP", cyng::TC_BOOL, "if true use a DHCP server"}},	//	[bool] 
		{ smf::OBIS_CUSTOM_IF_DHCP_LOCAL_IP_MASK, {"CUSTOM_IF_DHCP_LOCAL_IP_MASK", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_DHCP_DEFAULT_GW, {"CUSTOM_IF_DHCP_DEFAULT_GW", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_DHCP_DNS, {"CUSTOM_IF_DHCP_DNS", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_DHCP_START_ADDRESS, {"CUSTOM_IF_DHCP_START_ADDRESS", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 
		{ smf::OBIS_CUSTOM_IF_DHCP_END_ADDRESS, {"CUSTOM_IF_DHCP_END_ADDRESS", cyng::TC_IP_ADDRESS, ""}},	//	[IPv4/IPv6] 

		{ smf::OBIS_CUSTOM_IF_IP_CURRENT_1, {"CUSTOM_IF_IP_CURRENT_1", cyng::TC_IP_ADDRESS, "current IP address (customer interface)"}},	//	[IPv4/IPv6] 
		//{ smf::OBIS_CUSTOM_IF_IP_CURRENT_2, {"CUSTOM_IF_IP_CURRENT_2", cyng::TC_IP_ADDRESS, " current IP address (customer interface)"}},	//	[IPv4/IPv6]

		{ smf::OBIS_ROOT_CUSTOM_PARAM, {"ROOT_CUSTOM_PARAM", cyng::TC_NULL, "see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle "}},	//	
		{ smf::OBIS_ROOT_WAN, {"ROOT_WAN", cyng::TC_NULL, "see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status "}},	//	
		{ smf::OBIS_ROOT_WAN_PARAM, {"ROOT_WAN_PARAM", cyng::TC_NULL, "see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter "}},	//	
		{ smf::OBIS_ROOT_GSM, {"ROOT_GSM", cyng::TC_NULL, "see: Datenstruktur zum Lesen/Setzen der GSM Parameter "}},	//	
		{ smf::OBIS_ROOT_GPRS_PARAM, {"ROOT_GPRS_PARAM", cyng::TC_NULL, "see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter "}},	//	
		{ smf::OBIS_ROOT_GSM_STATUS, {"ROOT_GSM_STATUS", cyng::TC_NULL, ""}},
		{ smf::OBIS_PLC_STATUS, {"PLC_STATUS", cyng::TC_NULL, ""}},
		//{ smf::OBIS_PLC_STATUS, {"PLC_STATUS", cyng::TC_NULL, ""}},

		//{ smf::OBIS_W_MBUS_STATUS_MANUFACTURER, {"W_MBUS_STATUS_MANUFACTURER", cyng::TC_NULL, ""}},	//	[string] manufacturer of wireless M-Bus adapter
		//{ smf::OBIS_W_MBUS_STATUS_ID, {"W_MBUS_STATUS_ID", cyng::TC_NULL, ""}},	//	[string] adapter ID - see EN 13757-3/4
		//{ smf::OBIS_W_MBUS_STATUS_FIRMWARE, {"W_MBUS_STATUS_FIRMWARE", cyng::TC_NULL, ""}},	//	[string] adapter firmware version
		//{ smf::OBIS_W_MBUS_STATUS_HARDWARE, {"W_MBUS_STATUS_HARDWARE", cyng::TC_NULL, ""}},	//	[string] adapter hardware version

		{ smf::OBIS_ROOT_W_MBUS_STATUS, {"ROOT_W_MBUS_STATUS", cyng::TC_NULL, "see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status "}},	//	
		{ smf::OBIS_PROFILE_ADDRESS, {"PROFILE_ADDRESS", cyng::TC_NULL, ""}},
		{ smf::OBIS_PUSH_TARGET, {"PUSH_TARGET", cyng::TC_NULL, "push target name"}},	//	
		{ smf::OBIS_COMPUTER_NAME, {"COMPUTER_NAME", cyng::TC_NULL, ""}},
		{ smf::OBIS_LAN_DHCP_ENABLED, {"LAN_DHCP_ENABLED", cyng::TC_BOOL, ""}},	//	[bool]
		{ smf::OBIS_ROOT_LAN_DSL, {"ROOT_LAN_DSL", cyng::TC_NULL, "see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter"}},	//	
		{ smf::OBIS_ROOT_IPT_STATE, {"ROOT_IPT_STATE", cyng::TC_NULL, "see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status "}},	//	
		{ smf::OBIS_ROOT_IPT_PARAM, {"ROOT_IPT_PARAM", cyng::TC_NULL, "see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter "}},	//	

		//	ip-t status
		{ smf::OBIS_TARGET_IP_ADDRESS, {"TARGET_IP_ADDRESS", cyng::TC_IP_ADDRESS, "ip address of IP-T master "}},	//	[ip] 
		{ smf::OBIS_SOURCE_PORT, {"SOURCE_PORT", cyng::TC_UINT16, "target port of IP-T master "}},	//	[u16] 
		{ smf::OBIS_TARGET_PORT, {"TARGET_PORT", cyng::TC_UINT16, "source port of IP-T master "}},	//	[u16] 

		{ smf::OBIS_IPT_ACCOUNT, {"IPT_ACCOUNT", cyng::TC_STRING, ""}},	//	[s]
		{ smf::OBIS_IPT_PASSWORD, {"IPT_PASSWORD", cyng::TC_STRING, ""}},	//	[s]
		{ smf::OBIS_IPT_SCRAMBLED, {"IPT_SCRAMBLED", cyng::TC_NULL, ""}},	//	[bool] not standard!

		{ smf::OBIS_PEER_OBISLOG, {"PEER_OBISLOG", cyng::TC_NULL, "peer address: OBISLOG"}},	//	
		{ smf::OBIS_PEER_SCM, {"PEER_SCM", cyng::TC_NULL, "peer address: SCM (power)"}},	//	
		{ smf::OBIS_PEER_USERIF, {"PEER_USERIF", cyng::TC_NULL, "peer address: USERIF"}},	//	
		{ smf::OBIS_PEER_ADDRESS_WANGSM, {"PEER_ADDRESS_WANGSM", cyng::TC_NULL, "peer address: WAN/GSM"}},	//	
		{ smf::OBIS_PEER_ADDRESS, {"PEER_ADDRESS", cyng::TC_NULL, "unit is 255"}},	//	

		{ smf::OBIS_VERSION, {"VERSION", cyng::TC_STRING, "(0.2.0) firmware revision"}},		//	[string] 
		{ smf::OBIS_FILE_NAME, {"FILE_NAME", cyng::TC_STRING, ""}},	//	[string]
		{ smf::OBIS_MSG_COUNTER, {"MSG_COUNTER", cyng::TC_UINT32, "Anzahl aller Nachrichten zur Übertragung des Binary"}},	//	[u32] 
		{ smf::OBIS_LAST_MSG, {"LAST_MSG", cyng::TC_UINT32, "Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary"}},	//	[u32] 
		{ smf::OBIS_MSG_NUMBER, {"MSG_NUMBER", cyng::TC_NULL, ""}},
		{ smf::OBIS_BLOCK_NUMBER, {"BLOCK_NUMBER", cyng::TC_NULL, ""}},
		{ smf::OBIS_BINARY_DATA, {"BINARY_DATA", cyng::TC_NULL, ""}},

		{ smf::OBIS_ROOT_VISIBLE_DEVICES, {"ROOT_VISIBLE_DEVICES", cyng::TC_NULL, "visible devices (Liste der sichtbaren Sensoren)"}},	//	
		{ smf::OBIS_CODE_LIST_1_VISIBLE_DEVICES, {"CODE_LIST_1_VISIBLE_DEVICES", cyng::TC_NULL, "1. Liste der sichtbaren Sensoren"}},	//	
		{ smf::OBIS_ROOT_NEW_DEVICES, {"ROOT_NEW_DEVICES", cyng::TC_NULL, "new active devices"}},	//	
		{ smf::OBIS_ROOT_INVISIBLE_DEVICES, {"ROOT_INVISIBLE_DEVICES", cyng::TC_NULL, "not longer visible devices"}},	//	
		{ smf::OBIS_ROOT_ACTIVE_DEVICES, {"ROOT_ACTIVE_DEVICES", cyng::TC_NULL, "active devices (Liste der aktiven Sensoren)"}},	//	
		{ smf::OBIS_CODE_LIST_1_ACTIVE_DEVICES, {"CODE_LIST_1_ACTIVE_DEVICES", cyng::TC_NULL, "1. Liste der aktiven Sensoren)"}},	//	
		{ smf::OBIS_ACTIVATE_DEVICE, {"ACTIVATE_DEVICE", cyng::TC_NULL, "activate meter/sensor"}},	//	
		{ smf::OBIS_DEACTIVATE_DEVICE, {"DEACTIVATE_DEVICE", cyng::TC_NULL, "deactivate meter/sensor"}},	//	
		{ smf::OBIS_DELETE_DEVICE, {"DELETE_DEVICE", cyng::TC_NULL, "delete meter/sensor"}},	//	
		{ smf::OBIS_ROOT_DEVICE_INFO, {"ROOT_DEVICE_INFO", cyng::TC_NULL, "extended device information"}},	//	
		{ smf::OBIS_ROOT_ACCESS_RIGHTS, {"ROOT_ACCESS_RIGHTS", cyng::TC_NULL, "see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte "}},	//	

		{ smf::OBIS_ACCESS_USER_NAME, {"ACCESS_USER_NAME", cyng::TC_NULL, "user name for access"}},	//	
		{ smf::OBIS_ACCESS_PASSWORD, {"ACCESS_PASSWORD", cyng::TC_NULL, "SHA256 encrypted"}},	//	
		{ smf::OBIS_ACCESS_PUBLIC_KEY, {"ACCESS_PUBLIC_KEY", cyng::TC_NULL, ""}},

		{ smf::OBIS_ROOT_FILE_TRANSFER, {"ROOT_FILE_TRANSFER", cyng::TC_NULL, "7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) "}},	//	
		{ smf::OBIS_DATA_FIRMWARE, {"DATA_FIRMWARE", cyng::TC_NULL, ""}},
		{ smf::OBIS_DATA_FILENAME, {"DATA_FILENAME", cyng::TC_NULL, ""}},
		{ smf::OBIS_DATA_FILENAME_INDIRECT, {"DATA_FILENAME_INDIRECT", cyng::TC_NULL, ""}},
		{ smf::OBIS_DATA_APPLICATION, {"DATA_APPLICATION", cyng::TC_NULL, ""}},
		{ smf::OBIS_DATA_APPLICATION_INDIRECT, {"DATA_APPLICATION_INDIRECT", cyng::TC_NULL, ""}},
		{ smf::OBIS_DATA_PUSH_DETAILS, {"DATA_PUSH_DETAILS", cyng::TC_STRING, ""}},	// string

		{ smf::OBIS_DATA_IP_ADDRESS, {"DATA_IP_ADDRESS", cyng::TC_NULL, ""}},

		{ smf::OBIS_ROOT_DEVICE_IDENT, {"ROOT_DEVICE_IDENT", cyng::TC_NULL, "see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) "}},	//	
		{ smf::OBIS_DEVICE_CLASS, {"DEVICE_CLASS", cyng::TC_NULL, "Geräteklasse (OBIS code or '2D 2D 2D')"}},	//	
		{ smf::OBIS_DATA_MANUFACTURER, {"DATA_MANUFACTURER", cyng::TC_STRING, "FLAG"}},	//	[string] 
		{ smf::OBIS_SERVER_ID, {"SERVER_ID", cyng::TC_NULL, "Server ID "}},		//	
		{ smf::OBIS_DATA_PUBLIC_KEY, {"DATA_PUBLIC_KEY", cyng::TC_NULL, ""}},
		{ smf::OBIS_ROOT_FIRMWARE, {"ROOT_FIRMWARE", cyng::TC_NULL, "Firmware"}},	//	
		{ smf::OBIS_HARDWARE_FEATURES, {"HARDWARE_FEATURES", cyng::TC_NULL, "hardware equipment (charge, type, ...) 81 81 C7 82 0A NN"}},	//	

		{ smf::OBIS_DEVICE_KERNEL, {"DEVICE_KERNEL", cyng::TC_STRING, ""}},	//	[string]
		{ smf::OBIS_DEVICE_ACTIVATED, {"DEVICE_ACTIVATED", cyng::TC_NULL, ""}},

		//	device classes
		{ smf::OBIS_DEV_CLASS_BASIC_DIRECT, {"DEV_CLASS_BASIC_DIRECT", cyng::TC_NULL, "3 x 230 /400 V and 5 (100) A "}},	//	
		{ smf::OBIS_DEV_CLASS_BASIC_SEMI, {"DEV_CLASS_BASIC_SEMI", cyng::TC_NULL, "3 x 230 /400 V and 1 (6) A"}},	//	
		{ smf::OBIS_DEV_CLASS_BASIC_INDIRECT, {"DEV_CLASS_BASIC_INDIRECT", cyng::TC_NULL, "3 x  58 / 100 V and 1 (6) A "}},	//	
		{ smf::OBIS_DEV_CLASS_IW, {"DEV_CLASS_IW", cyng::TC_NULL, "IW module"}},		//	
		{ smf::OBIS_DEV_CLASS_PSTN, {"DEV_CLASS_PSTN", cyng::TC_NULL, "PSTN/GPRS"}},	//	
		{ smf::OBIS_DEV_CLASS_GPRS, {"DEV_CLASS_GPRS", cyng::TC_NULL, "GPRS/PLC"}},	//	
		{ smf::OBIS_DEV_CLASS_KM, {"DEV_CLASS_KM", cyng::TC_NULL, "KM module (LAN/DSL)"}},		//	
		{ smf::OBIS_DEV_CLASS_NK, {"DEV_CLASS_NK", cyng::TC_NULL, "NK/HS"}},		//	
		{ smf::OBIS_DEV_CLASS_EXTERN, {"DEV_CLASS_EXTERN", cyng::TC_NULL, "external load profile collector "}},	//	
		{ smf::OBIS_DEV_CLASS_GW, {"DEV_CLASS_GW", cyng::TC_NULL, "gateway"}},		//	
		{ smf::OBIS_DEV_CLASS_LAN, {"DEV_CLASS_LAN", cyng::TC_NULL, "see DEV_CLASS_MUC_LAN"}},	//	
		{ smf::OBIS_DEV_CLASS_eHZ, {"DEV_CLASS_eHZ", cyng::TC_NULL, ""}},
		{ smf::OBIS_DEV_CLASS_3HZ, {"DEV_CLASS_3HZ", cyng::TC_NULL, ""}},
		{ smf::OBIS_DEV_CLASS_MUC_LAN, {"DEV_CLASS_MUC_LAN", cyng::TC_NULL, "(MUC-LAN/DSL)"}},	// 

		{ smf::OBIS_REBOOT, {"REBOOT", cyng::TC_NULL, ""}},	//	request reboot
		{ smf::OBIS_UPDATE_FW, {"UPDATE_FW", cyng::TC_NULL, ""}},	//	activate firmware

		{ smf::OBIS_ROOT_SENSOR_PARAMS, {"ROOT_SENSOR_PARAMS", cyng::TC_NULL, "data mirror root element (Eigenschaften eines Datenspiegels)"}},	//	
		{ smf::OBIS_ROOT_SENSOR_BITMASK, {"ROOT_SENSOR_BITMASK", cyng::TC_UINT16, " Bitmask to define bits that will be transferred into log"}},	//	[u16]
		{ smf::OBIS_AVERAGE_TIME_MS, {"AVERAGE_TIME_MS", cyng::TC_NULL, "average time between two received data records (milliseconds)"}},	//	
		{ smf::OBIS_ROOT_DATA_COLLECTOR, {"ROOT_DATA_COLLECTOR", cyng::TC_NULL, "data collector root element (Eigenschaften eines Datensammlers)"}},	//	
		{ smf::OBIS_DATA_COLLECTOR_ACTIVE, {"DATA_COLLECTOR_ACTIVE", cyng::TC_BOOL, ""}},	//	true/false
		{ smf::OBIS_DATA_COLLECTOR_SIZE, {"DATA_COLLECTOR_SIZE", cyng::TC_NULL, "max. table size"}},		//	
		{ smf::OBIS_TIME_REFERENCE, {"TIME_REFERENCE", cyng::TC_UINT8, "0 == UTC, 1 == UTC + time zone, 2 == local time"}},	//	[u8] 
		{ smf::OBIS_DATA_REGISTER_PERIOD, {"DATA_REGISTER_PERIOD", cyng::TC_UINT32, "register period in seconds (0 == event driven)"}},	//	 (u32) 
		{ smf::OBIS_ROOT_NTP, {"ROOT_NTP", cyng::TC_NULL, ""}},	//	NTP configuration
		{ smf::OBIS_CODE_NTP_SERVER, {"CODE_NTP_SERVER", cyng::TC_NULL, "List of NTP servers"}},	//	
		{ smf::OBIS_CODE_NTP_PORT, {"CODE_NTP_PORT", cyng::TC_UINT16, "NTP port (123)"}},
		{ smf::OBIS_CODE_NTP_TZ, {"CODE_NTP_TZ", cyng::TC_UINT32, "timezone"}},
		{ smf::OBIS_CODE_NTP_OFFSET, {"CODE_NTP_OFFSET", cyng::TC_NULL, "Offset to transmission of the signal for synchronization"}},	//	[sec] 
		{ smf::OBIS_CODE_NTP_ACTIVE, {"CODE_NTP_ACTIVE", cyng::TC_NULL, "NTP enabled/disables"}},	//	[bool] 
		{ smf::OBIS_ROOT_DEVICE_TIME, {"ROOT_DEVICE_TIME", cyng::TC_NULL, "device time"}},	//	

		{ smf::OBIS_ROOT_IF, {"ROOT_IF", cyng::TC_NULL, ""}},	 //	root of all interfaces
		//{ smf::OBIS_ROOT_IF_, {"ROOT_IF_", cyng::TC_NULL, ""}},?	
		//{ smf::OBIS_ROOT_IF_, {"ROOT_IF_", cyng::TC_NULL, ""}},?	

		{ smf::OBIS_IF_1107, {"IF_1107", cyng::TC_NULL, ""}},	 //	1107 interface (IEC 62056-21)
		{ smf::OBIS_IF_1107_ACTIVE, {"IF_1107_ACTIVE", cyng::TC_BOOL, " if true 1107 interface active otherwise SML interface active"}},
		{ smf::OBIS_IF_1107_LOOP_TIME, {"IF_1107_LOOP_TIME", cyng::TC_NULL, "Loop timeout in seconds"}}, 
		{ smf::OBIS_IF_1107_RETRIES, {"IF_1107_RETRIES", cyng::TC_NULL, "Retry count"}}, //	(u) - 
		{ smf::OBIS_IF_1107_MIN_TIMEOUT, {"IF_1107_MIN_TIMEOUT", cyng::TC_NULL, "Minimal answer timeout(300)"}}, //	(u) - 
		{ smf::OBIS_IF_1107_MAX_TIMEOUT, {"IF_1107_MAX_TIMEOUT", cyng::TC_NULL, " Maximal answer timeout(5000)"}}, //	(u) -
		{ smf::OBIS_IF_1107_MAX_DATA_RATE, {"IF_1107_MAX_DATA_RATE", cyng::TC_NULL, "Maximum data bytes(10240)"}}, //	(u) - 
		{ smf::OBIS_IF_1107_RS485, {"IF_1107_RS485", cyng::TC_BOOL, "if true RS 485, otherwise RS 323"}}, //	(bool) - 
		{ smf::OBIS_IF_1107_PROTOCOL_MODE, {"IF_1107_PROTOCOL_MODE", cyng::TC_UINT8, "Protocol mode(A ... D)"}}, //	(u) - 
		{ smf::OBIS_IF_1107_METER_LIST, {"IF_1107_METER_LIST", cyng::TC_NULL, "Liste der abzufragenden 1107 Zähler"}}, // 
		{ smf::OBIS_IF_1107_AUTO_ACTIVATION, {"IF_1107_AUTO_ACTIVATION", cyng::TC_NULL, ""}}, //(True)
		{ smf::OBIS_IF_1107_TIME_GRID, {"IF_1107_TIME_GRID", cyng::TC_NULL, "time grid of load profile readout in seconds"}}, //	
		{ smf::OBIS_IF_1107_TIME_SYNC, {"IF_1107_TIME_SYNC", cyng::TC_NULL, "time sync in seconds"}}, //	
		{ smf::OBIS_IF_1107_MAX_VARIATION, {"IF_1107_MAX_VARIATION", cyng::TC_NULL, "seconds"}}, //(seconds)

		{ smf::OBIS_IF_1107_METER_ID, {"IF_1107_METER_ID", cyng::TC_STRING, ""}}, //	(string)
		{ smf::OBIS_IF_1107_BAUDRATE, {"IF_1107_BAUDRATE", cyng::TC_NULL, ""}}, //	(u)
		{ smf::OBIS_IF_1107_ADDRESS, {"IF_1107_ADDRESS", cyng::TC_STRING, ""}}, //	(string)
		{ smf::OBIS_IF_1107_P1, {"IF_1107_P1", cyng::TC_STRING, ""}}, //	(string)
		{ smf::OBIS_IF_1107_W5, {"IF_1107_W5", cyng::TC_STRING, ""}}, //	(string)

																								  //
		//	Interfaces
		//
		{ smf::OBIS_IF_LAN_DSL, {"IF_LAN_DSL", cyng::TC_NULL, "see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter"}},	// 
		{ smf::OBIS_CODE_IF_LAN_ADDRESS, {"CODE_IF_LAN_ADDRESS", cyng::TC_IP_ADDRESS, "IPv4 or IPv6 address"}},	// 
		{ smf::OBIS_CODE_IF_LAN_SUBNET_MASK, {"CODE_IF_LAN_SUBNET_MASK", cyng::TC_IP_ADDRESS, ""}},	// IPv4 or IPv6
		{ smf::OBIS_CODE_IF_LAN_GATEWAY, {"CODE_IF_LAN_GATEWAY", cyng::TC_IP_ADDRESS, ""}},	// IPv4 or IPv6
		{ smf::OBIS_CODE_IF_LAN_DNS_PRIMARY, {"CODE_IF_LAN_DNS_PRIMARY", cyng::TC_IP_ADDRESS, ""}},	// IPv4 or IPv6
		{ smf::OBIS_CODE_IF_LAN_DNS_SECONDARY, {"CODE_IF_LAN_DNS_SECONDARY", cyng::TC_IP_ADDRESS, ""}},	// IPv4 or IPv6
		{ smf::OBIS_CODE_IF_LAN_DNS_TERTIARY, {"CODE_IF_LAN_DNS_TERTIARY", cyng::TC_IP_ADDRESS, ""}},	// IPv4 or IPv6

		{ smf::OBIS_IF_EDL, {"IF_EDL", cyng::TC_NULL, "M-Bus EDL (RJ10)"}},		//	

		{ smf::OBIS_IF_EDL_PROTOCOL, {"IF_EDL_PROTOCOL", cyng::TC_UINT8, "always 1"}},
		{ smf::OBIS_IF_EDL_BAUDRATE, {"IF_EDL_BAUDRATE", cyng::TC_NULL, "0 = auto, 6 = 9600, 10 = 115200 baud"}},	

		{ smf::OBIS_IF_wMBUS, {"IF_wMBUS", cyng::TC_NULL, "Wireless M-BUS"}},
		{ smf::OBIS_IF_PLC, {"IF_PLC", cyng::TC_NULL, ""}},
		//{ smf::OBIS_CODE_IF_SyM2, {"CODE_IF_SyM2", cyng::TC_NULL, ""}},	//	same as IF_EDL
		{ smf::OBIS_ACT_SENSOR_TIME, {"ACT_SENSOR_TIME", cyng::TC_NULL, "actSensorTime - current UTC time"}},	//	[SML_Time] 
		{ smf::OBIS_TZ_OFFSET, {"TZ_OFFSET", cyng::TC_UINT16, "offset to actual time zone in minutes (-720 .. +720)"}},

		{ smf::OBIS_CLASS_OP_LOG, {"CLASS_OP_LOG", cyng::TC_NULL, ""}},
		{ smf::OBIS_CLASS_EVENT, {"CLASS_EVENT", cyng::TC_UINT32, "event"}},

		{ smf::OBIS_INTERFACE_01_NAME, {"INTERFACE_01_NAME", cyng::TC_STRING, "interface name"}},
		{ smf::OBIS_INTERFACE_02_NAME, {"INTERFACE_02_NAME", cyng::TC_STRING, "interface name"}},
		{ smf::OBIS_INTERFACE_03_NAME, {"INTERFACE_03_NAME", cyng::TC_STRING, "interface name"}},
		{ smf::OBIS_INTERFACE_04_NAME, {"INTERFACE_04_NAME", cyng::TC_STRING, "interface name"}},

		//
		//	wMBus - 81 06 0F 06 00 FF
		//	7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
		//
		{ smf::OBIS_W_MBUS_ADAPTER_MANUFACTURER, {"W_MBUS_ADAPTER_MANUFACTURER", cyng::TC_NULL, ""}},	//	[string] 
		{ smf::OBIS_W_MBUS_ADAPTER_ID, {"W_MBUS_ADAPTER_ID", cyng::TC_NULL, ""}},
		{ smf::OBIS_W_MBUS_FIRMWARE, {"W_MBUS_FIRMWARE", cyng::TC_STRING, ""}},	//	[string]
		{ smf::OBIS_W_MBUS_HARDWARE, {"W_MBUS_HARDWARE", cyng::TC_STRING, ""}},	//	[string]
		{ smf::OBIS_W_MBUS_FIELD_STRENGTH, {"W_MBUS_FIELD_STRENGTH", cyng::TC_UINT16, "dbm"}},	//	[u16] 
		{ smf::OBIS_W_MBUS_LAST_RECEPTION, {"W_MBUS_LAST_RECEPTION", cyng::TC_NULL, "Time since last radio telegram reception"}},	//	[seconds] 

		//
		//	Wireless M-BUS config
		//	81 06 19 07 00 FF, IF_wMBUS
		//
		{ smf::OBIS_W_MBUS_PROTOCOL, {"W_MBUS_PROTOCOL", cyng::TC_NULL, ""}},
		{ smf::OBIS_W_MBUS_MODE_S, {"W_MBUS_MODE_S", cyng::TC_NULL, ""}},	//	u8
		{ smf::OBIS_W_MBUS_MODE_T, {"W_MBUS_MODE_T", cyng::TC_NULL, ""}},	//	u8

		{ smf::OBIS_W_MBUS_REBOOT, {"W_MBUS_REBOOT", cyng::TC_UINT32, ""}},	//	u32 (seconds)
		{ smf::OBIS_W_MBUS_POWER, {"W_MBUS_POWER", cyng::TC_UINT8, "0 = default, 1 = low, 2 = medium, 3 = high"}},		//	
		{ smf::OBIS_W_MBUS_INSTALL_MODE, {"W_MBUS_INSTALL_MODE", cyng::TC_NULL, ""}},
		{ smf::OBIS_W_MBUS_MAX_MSG_TIMEOUT, {"W_MBUS_MAX_MSG_TIMEOUT", cyng::TC_UINT8, ""}},	//	u8 (seconds)
		{ smf::OBIS_W_MBUS_MAX_SML_TIMEOUT_IN, {"W_MBUS_MAX_SML_TIMEOUT_IN", cyng::TC_UINT16, "max timeout between SML close request and SML open response from device to gateway"}},
		//	u16 (seconds) 
		{ smf::OBIS_W_MBUS_MAX_SML_TIMEOUT_OUT, {"W_MBUS_MAX_SML_TIMEOUT_OUT", cyng::TC_UINT16, "max timeout between SML close request and SML open response from gateway to device"}},

		//	Spannung - voltage
		//	Strom - current
		//	Leistungsfaktor - power factor
		//	Scheinleistung - apparent power
		//	Wirkleistung - effective (or active) power
		{ smf::OBIS_REG_POS_ACT_MAX_DEMAND_NO_TARIFF, {"REG_POS_ACT_MAX_DEMAND_NO_TARIFF", cyng::TC_NULL, "1.6.0 - Positive active maximum demand (A+) total [kW]"}},	//	

		{ smf::OBIS_REG_NEG_ACT_MAX_DEMAND_NO_TARIFF, {"REG_NEG_ACT_MAX_DEMAND_NO_TARIFF", cyng::TC_NULL, "2.6.0 - Negative active maximum demand (A-) total [kW]"}},	//	

		{ smf::OBIS_REG_POS_REACT_MAX_DEMAND_NO_TARIFF, {"REG_POS_REACT_MAX_DEMAND_NO_TARIFF", cyng::TC_NULL, "3.6.0 - Positive reactive maximum demand (Q+) total [kvar]"}},	//	

		{ smf::OBIS_REG_NEG_REACT_MAX_DEMAND_NO_TARIFF, {"REG_NEG_REACT_MAX_DEMAND_NO_TARIFF", cyng::TC_NULL, "4.6.0 - Negative reactive maximum demand (Q-) total [kvar]"}},	//	

		{ smf::OBIS_REG_POS_AE_NO_TARIFF, {"REG_POS_AE_NO_TARIFF", cyng::TC_NULL, "1.8.0 - Zählwerk pos. Wirkenergie,	tariflos - Positive active energy (A+) total [kWh]"}},	//	
		{ smf::OBIS_REG_POS_AE_T1, {"REG_POS_AE_T1", cyng::TC_NULL, "1.8.1 - Zählwerk pos. Wirkenergie, Tarif 1 - Positive active energy (A+) in tariff T1 [kWh]"}},		//	
		{ smf::OBIS_REG_POS_AE_T2, {"REG_POS_AE_T2", cyng::TC_NULL, "1.8.2 - Zählwerk pos. Wirkenergie, Tarif 2 - Positive active energy (A+) in tariff T2 [kWh]"}},		//	
		{ smf::OBIS_REG_POS_AE_T3, {"REG_POS_AE_T3", cyng::TC_NULL, "1.8.3 - Zählwerk pos. Wirkenergie, Tarif 3 - Positive active energy (A+) in tariff T3 [kWh]"}},		//	
		{ smf::OBIS_REG_POS_AE_T4, {"REG_POS_AE_T4", cyng::TC_NULL, "1.8.4 - Zählwerk pos. Wirkenergie, Tarif 4 - Positive active energy (A+) in tariff T4 [kWh]"}},		//	

		{ smf::OBIS_REG_NEG_AE_NO_TARIFF, {"REG_NEG_AE_NO_TARIFF", cyng::TC_NULL, "2.8.0 - Zählwerk neg. Wirkenergie, tariflos - Negative active energy (A+) total [kWh]"}},	//	
		{ smf::OBIS_REG_NEG_AE_T1, {"REG_NEG_AE_T1", cyng::TC_NULL, "2.8.1 - Zählwerk neg. Wirkenergie, Tarif 1 - Negative active energy (A+) in tariff T1 [kWh]"}},		//	
		{ smf::OBIS_REG_NEG_AE_T2, {"REG_NEG_AE_T2", cyng::TC_NULL, "2.8.2 - Zählwerk neg. Wirkenergie, Tarif 2 - Negative active energy (A+) in tariff T2 [kWh]"}},		//	
		{ smf::OBIS_REG_NEG_AE_T3, {"REG_NEG_AE_T3", cyng::TC_NULL, "2.8.3 - Zählwerk neg. Wirkenergie, Tarif 3 - Negative active energy (A+) in tariff T3 [kWh]"}},		//	
		{ smf::OBIS_REG_NEG_AE_T4, {"REG_NEG_AE_T4", cyng::TC_NULL, "2.8.4 - Zählwerk neg. Wirkenergie, Tarif 4 - Negative active energy (A+) in tariff T4 [kWh]"}},		//	

		{ smf::OBIS_REG_POS_RE_NO_TARIFF, {"REG_POS_RE_NO_TARIFF", cyng::TC_NULL, "3.8.0 - Positive reactive energy (Q+) total [kvarh]"}},	//	
		{ smf::OBIS_REG_POS_RE_NO_T1, {"REG_POS_RE_NO_T1", cyng::TC_NULL, "3.8.1 - Positive reactive energy (Q+) in tariff T1 [kvarh]"}},		//	
		{ smf::OBIS_REG_POS_RE_NO_T2, {"REG_POS_RE_NO_T2", cyng::TC_NULL, "3.8.2 - Positive reactive energy (Q+) in tariff T2 [kvarh]"}},		//	
		{ smf::OBIS_REG_POS_RE_NO_T3, {"REG_POS_RE_NO_T3", cyng::TC_NULL, "3.8.3 - Positive reactive energy (Q+) in tariff T3 [kvarh]"}},		//	
		{ smf::OBIS_REG_POS_RE_NO_T4, {"REG_POS_RE_NO_T4", cyng::TC_NULL, "3.8.4 - Positive reactive energy (Q+) in tariff T4 [kvarh]"}},		//	

		{ smf::OBIS_REG_NEG_RE_NO_TARIFF, {"REG_NEG_RE_NO_TARIFF", cyng::TC_NULL, "4.8.0 - Negative reactive energy (Q-) total [kvarh]"}},	//	
		{ smf::OBIS_REG_NEG_RE_NO_T1, {"REG_NEG_RE_NO_T1", cyng::TC_NULL, "4.8.1 - Negative reactive energy (Q-) in tariff T1 [kvarh]"}},		//	
		{ smf::OBIS_REG_NEG_RE_NO_T2, {"REG_NEG_RE_NO_T2", cyng::TC_NULL, "4.8.2 - Negative reactive energy (Q-) in tariff T2 [kvarh]"}},		//	
		{ smf::OBIS_REG_NEG_RE_NO_T3, {"REG_NEG_RE_NO_T3", cyng::TC_NULL, "4.8.3 - Negative reactive energy (Q-) in tariff T3 [kvarh]"}},		//	
		{ smf::OBIS_REG_NEG_RE_NO_T4, {"REG_NEG_RE_NO_T4", cyng::TC_NULL, "4.8.4 - Negative reactive energy (Q-) in tariff T4 [kvarh]"}},		//	

		{ smf::OBIS_REG_IMP_IND_RE_NO_TARIFF, {"REG_IMP_IND_RE_NO_TARIFF", cyng::TC_NULL, "5.8.0 - Imported inductive reactive energy in 1-st quadrant (Q1) total [kvarh]"}},	//	

		{ smf::OBIS_REG_IMP_CAP_RE_NO_TARIFF, {"REG_IMP_CAP_RE_NO_TARIFF", cyng::TC_NULL, "6.8.0 - Imported capacitive reactive energy in 2-nd quadrant (Q2) total [kvarh]"}},	//	

		{ smf::OBIS_REG_EXP_IND_RE_NO_TARIFF, {"REG_EXP_IND_RE_NO_TARIFF", cyng::TC_NULL, "7.8.0 - Exported inductive reactive energy in 3-rd quadrant (Q3) total [kvarh]"}},	//	

		{ smf::OBIS_REG_EXP_CAP_RE_NO_TARIFF, {"REG_EXP_CAP_RE_NO_TARIFF", cyng::TC_NULL, "8.8.0 - Exported capacitive reactive energy in 4-th quadrant (Q4) total [kvarh]"}},	//	

		{ smf::OBIS_REG_APPARENT_E_NO_TARIFF, {"REG_APPARENT_E_NO_TARIFF", cyng::TC_NULL, "9.8.0 - Apparent energy (S+) total [kVAh]"}},	//	

		{ smf::OBIS_REG_CUR_POS_AE, {"REG_CUR_POS_AE", cyng::TC_NULL, "15.7.0 - Aktuelle positive Wirkleistung, Betrag"}},		//	

		{ smf::OBIS_REG_ABS_AE_NO_TARIFF, {"REG_ABS_AE_NO_TARIFF", cyng::TC_NULL, "15.8.0 - Absolute active energy (A+) total [kWh]"}},	//	
		{ smf::OBIS_REG_ABS_AE_T1, {"REG_ABS_AE_T1", cyng::TC_NULL, "15.8.1 - Absolute active energy (A+) in tariff T1 [kWh]"}},		//	
		{ smf::OBIS_REG_ABS_AE_T2, {"REG_ABS_AE_T2", cyng::TC_NULL, "15.8.2 - Absolute active energy (A+) in tariff T2 [kWh]"}},		//	
		{ smf::OBIS_REG_ABS_AE_T3, {"REG_ABS_AE_T3", cyng::TC_NULL, "15.8.3 - Absolute active energy (A+) in tariff T3 [kWh]"}},		//	
		{ smf::OBIS_REG_ABS_AE_T4, {"REG_ABS_AE_T4", cyng::TC_NULL, "15.8.4 - Absolute active energy (A+) in tariff T4 [kWh]"}},		//	

		{ smf::OBIS_REG_CUR_AP, {"REG_CUR_AP", cyng::TC_NULL, ""}},		//	aktuelle Wirkleistung 

		//	01 00 24 07 00 FF: Wirkleistung L1
		//01 00 38 07 00 FF: Wirkleistung L2
		//01 00 4C 07 00 FF: Wirkleistung L3
		//01 00 60 32 00 02: Aktuelle Chiptemperatur
		//01 00 60 32 00 03: Minimale Chiptemperatur
		//01 00 60 32 00 04: Maximale Chiptemperatur
		//01 00 60 32 00 05: Gemittelte Chiptemperatur
		{ smf::OBIS_REG_CURRENT_L1, {"REG_CURRENT_L1", cyng::TC_NULL, "Strom L1"}},	
		{ smf::OBIS_REG_VOLTAGE_L1, {"REG_VOLTAGE_L1", cyng::TC_NULL, "Spannung L1"}},
		{ smf::OBIS_REG_NEG_REACTIVE_INST_POWER_L1, {"REG_NEG_REACTIVE_INST_POWER_L1", cyng::TC_NULL, "Negative reactive instantaneous power (Q-) in phase L1 [kvar]"}},
		{ smf::OBIS_REG_CURRENT_L2, {"REG_CURRENT_L2", cyng::TC_NULL, "Strom L2"}},	
		{ smf::OBIS_REG_VOLTAGE_L2, {"REG_VOLTAGE_L2", cyng::TC_NULL, "Spannung L2"}},
		{ smf::OBIS_REG_NEG_REACTIVE_INST_POWER_L2, {"REG_NEG_REACTIVE_INST_POWER_L2", cyng::TC_NULL, "Negative reactive instantaneous power (Q-) in phase L2 [kvar]"}},		
		{ smf::OBIS_REG_CURRENT_L3, {"REG_CURRENT_L3", cyng::TC_NULL, "Strom L3"}},	
		{ smf::OBIS_REG_VOLTAGE_L3, {"REG_VOLTAGE_L3", cyng::TC_NULL, "Spannung L3 "}},	
		{ smf::OBIS_REG_VOLTAGE_MIN, {"REG_VOLTAGE_MIN", cyng::TC_NULL, "Spannungsminimum"}},
		{ smf::OBIS_REG_VOLTAGE_MAX, {"REG_VOLTAGE_MAX", cyng::TC_NULL, "Spannungsmaximum"}},
		{ smf::OBIS_REG_NEG_REACTIVE_INST_POWER_L3, {"REG_NEG_REACTIVE_INST_POWER_L3", cyng::TC_NULL, "Negative reactive instantaneous power (Q-) in phase L3 [kvar]"}},


		/**
		 *	Statuswort (per Schreiben ist das Rücksetzen ausgewählter Statusbits) Unsigned64
		 *	see also http://www.sagemcom.com/fileadmin/user_upload/Energy/Dr.Neuhaus/Support/ZDUE-MUC/Doc_communs/ZDUE-MUC_Anwenderhandbuch_V2_5.pdf
		 *	bit meaning
		 *	0	always 0
		 *	1	always 1
		 *	2-7	always 0
		 *	8	1 if (fatal) error was detected
		 *	9	1 if restart was triggered by watchdog reset
		 *	10	0 if IP address is available (DHCP)
		 *	11	0 if ethernet link is available
		 *	12	always 0
		 *	13	0 if authorized on IP-T server
		 *	14	1 in case of out of memory
		 *	15	always 0
		 *	16	1 if Service interface is available (Kundenschnittstelle)
		 *	17	1 if extension interface is available (Erweiterungs-Schnittstelle)
		 *	18	1 if Wireless M-Bus interface is available
		 *	19	1 if PLC is available
		 *	20-31	always 0
		 *	32	1 if time base is unsure
		 */
		{ smf::OBIS_CLASS_OP_LOG_STATUS_WORD, {"CLASS_OP_LOG_STATUS_WORD", cyng::TC_NULL, ""}},
		{ smf::OBIS_CLASS_OP_LOG_FIELD_STRENGTH, {"CLASS_OP_LOG_FIELD_STRENGTH", cyng::TC_NULL, ""}},
		{ smf::OBIS_CLASS_OP_LOG_CELL, {"CLASS_OP_LOG_CELL", cyng::TC_UINT16, "aktuelle Zelleninformation"}},
		{ smf::OBIS_CLASS_OP_LOG_AREA_CODE, {"CLASS_OP_LOG_AREA_CODE", cyng::TC_UINT16, "aktueller Location - oder Areacode"}},
		{ smf::OBIS_CLASS_OP_LOG_PROVIDER, {"CLASS_OP_LOG_PROVIDER", cyng::TC_UINT32, "	aktueller Provider-Identifier(uint32)"}},	

		{ smf::OBIS_GSM_ADMISSIBLE_OPERATOR, {"GSM_ADMISSIBLE_OPERATOR", cyng::TC_NULL, ""}},

		//
		//	attention codes
		//
		{ smf::OBIS_ATTENTION_UNKNOWN_ERROR, {"ATTENTION_UNKNOWN_ERROR", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_UNKNOWN_SML_ID, {"ATTENTION_UNKNOWN_SML_ID", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_NOT_AUTHORIZED, {"ATTENTION_NOT_AUTHORIZED", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_NO_SERVER_ID, {"ATTENTION_NO_SERVER_ID", cyng::TC_NULL, "unable to find recipient for request"}},
		{ smf::OBIS_ATTENTION_NO_REQ_FIELD, {"ATTENTION_NO_REQ_FIELD", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_CANNOT_WRITE, {"ATTENTION_CANNOT_WRITE", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_CANNOT_READ, {"ATTENTION_CANNOT_READ", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_COMM_ERROR, {"ATTENTION_COMM_ERROR", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_PARSER_ERROR, {"ATTENTION_PARSER_ERROR", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_OUT_OF_RANGE, {"ATTENTION_OUT_OF_RANGE", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_NOT_EXECUTED, {"ATTENTION_NOT_EXECUTED", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_INVALID_CRC, {"ATTENTION_INVALID_CRC", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_NO_BROADCAST, {"ATTENTION_NO_BROADCAST", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_UNEXPECTED_MSG, {"ATTENTION_UNEXPECTED_MSG", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_UNKNOWN_OBIS_CODE, {"ATTENTION_UNKNOWN_OBIS_CODE", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_UNSUPPORTED_DATA_TYPE, {"ATTENTION_UNSUPPORTED_DATA_TYPE", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_ELEMENT_NOT_OPTIONAL, {"ATTENTION_ELEMENT_NOT_OPTIONAL", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_NO_ENTRIES, {"ATTENTION_NO_ENTRIES", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_END_LIMIT_BEFORE_START, {"ATTENTION_END_LIMIT_BEFORE_START", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_NO_ENTRIES_IN_RANGE, {"ATTENTION_NO_ENTRIES_IN_RANGE", cyng::TC_NULL, "range is empty - not the profile"}},
		{ smf::OBIS_ATTENTION_MISSING_CLOSE_MSG, {"ATTENTION_MISSING_CLOSE_MSG", cyng::TC_NULL, ""}},
		{ smf::OBIS_ATTENTION_OK, {"ATTENTION_OK", cyng::TC_NULL, "no error"}},
		{ smf::OBIS_ATTENTION_JOB_IS_RUNNINNG, {"ATTENTION_JOB_IS_RUNNINNG", cyng::TC_NULL, ""}},

		//
		//	source for log entries (OBIS-T)
		//
		{ smf::OBIS_LOG_SOURCE_ETH_AUX, {"LOG_SOURCE_ETH_AUX", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_ETH_CUSTOM, {"LOG_SOURCE_ETH_CUSTOM", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_RS232, {"LOG_SOURCE_RS232", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_ETH, {"LOG_SOURCE_ETH", cyng::TC_NULL, "WAN interface"}},
		{ smf::OBIS_LOG_SOURCE_eHZ, {"LOG_SOURCE_eHZ", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_wMBUS, {"LOG_SOURCE_wMBUS", cyng::TC_NULL, ""}},

		{ smf::OBIS_LOG_SOURCE_IP, {"LOG_SOURCE_IP", cyng::TC_NULL, ""}},

		{ smf::OBIS_LOG_SOURCE_SML_EXT, {"LOG_SOURCE_SML_EXT", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SML_CUSTOM, {"LOG_SOURCE_SML_CUSTOM", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SML_SERVICE, {"LOG_SOURCE_SML_SERVICE", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SML_WAN, {"LOG_SOURCE_SML_WAN", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SML_eHZ, {"LOG_SOURCE_SML_eHZ", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SML_wMBUS, {"LOG_SOURCE_SML_wMBUS", cyng::TC_NULL, ""}},

		{ smf::OBIS_LOG_SOURCE_PUSH_SML, {"LOG_SOURCE_PUSH_SML", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_PUSH_IPT_SOURCE, {"LOG_SOURCE_PUSH_IPT_SOURCE", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_PUSH_IPT_SINK, {"LOG_SOURCE_PUSH_IPT_SINK", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_WAN_DHCP, {"LOG_SOURCE_WAN_DHCP", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_WAN_IP, {"LOG_SOURCE_WAN_IP", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_WAN_PPPoE, {"LOG_SOURCE_WAN_PPPoE", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_WAN_IPT_CONTROLLER, {"LOG_SOURCE_WAN_IPT_CONTROLLER", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_WAN_IPT, {"LOG_SOURCE_WAN_IPT", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_WAN_NTP, {"LOG_SOURCE_WAN_NTP", cyng::TC_NULL, ""}},

		{ smf::OBIS_SET_START_FW_UPDATE, {"SET_START_FW_UPDATE", cyng::TC_NULL, ""}},
		{ smf::OBIS_SET_DISPATCH_FW_UPDATE, {"SET_DISPATCH_FW_UPDATE", cyng::TC_NULL, ""}},


		{ smf::OBIS_LOG_SOURCE_LOG, {"LOG_SOURCE_LOG", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SCM, {"LOG_SOURCE_SCM", cyng::TC_NULL, "internal software function"}},
		{ smf::OBIS_LOG_SOURCE_UPDATE, {"LOG_SOURCE_UPDATE", cyng::TC_NULL, ""}},
		{ smf::OBIS_LOG_SOURCE_SMLC, {"LOG_SOURCE_SMLC", cyng::TC_NULL, ""}},

		{ smf::OBIS_LOG_SOURCE_LEDIO, {"LOG_SOURCE_LEDIO", cyng::TC_NULL, ""}},

		{ smf::OBIS_LOG_SOURCE_WANPLC, {"LOG_SOURCE_WANPLC", cyng::TC_NULL, ""}},

		//
		//	SMF specific codes
		//
		{ smf::OBIS_ROOT_BROKER, {"ROOT_BROKER", cyng::TC_NULL, "90 00 00 00 00 NN - broker list"}},
		{ smf::OBIS_BROKER_LOGIN, {"BROKER_LOGIN", cyng::TC_NULL, ""}},

		{ smf::OBIS_BROKER_SERVER, {"BROKER_SERVER", cyng::TC_NULL, "ip address"}},
		{ smf::OBIS_BROKER_SERVICE, {"BROKER_SERVICE", cyng::TC_UINT16, "port"}},
		{ smf::OBIS_BROKER_USER, {"BROKER_USER", cyng::TC_NULL, ""}},
		{ smf::OBIS_BROKER_PWD, {"BROKER_PWD", cyng::TC_NULL, ""}},
		{ smf::OBIS_BROKER_TIMEOUT, {"BROKER_TIMEOUT", cyng::TC_NULL, ""}},

		{ smf::OBIS_BROKER_BLOCKLIST, {"BROKER_BLOCKLIST", cyng::TC_NULL, ""}},

		{ smf::OBIS_ROOT_SERIAL, {"ROOT_SERIAL", cyng::TC_NULL, ""}},
		{ smf::OBIS_SERIAL_NAME, {"SERIAL_NAME", cyng::TC_NULL, "example: /dev/ttyAPP0"}},
		{ smf::OBIS_SERIAL_DATABITS, {"SERIAL_DATABITS", cyng::TC_NULL, ""}},
		{ smf::OBIS_SERIAL_PARITY, {"SERIAL_PARITY", cyng::TC_NULL, ""}},
		{ smf::OBIS_SERIAL_FLOW_CONTROL, {"SERIAL_FLOW_CONTROL", cyng::TC_NULL, ""}},
		{ smf::OBIS_SERIAL_STOPBITS, {"SERIAL_STOPBITS", cyng::TC_NULL, ""}},
		{ smf::OBIS_SERIAL_SPEED, {"SERIAL_SPEED", cyng::TC_NULL, ""}},
		{ smf::OBIS_SERIAL_TASK, {"SERIAL_TASK", cyng::TC_NULL, "LMN port task"}},

		{ smf::OBIS_ROOT_NMS, {"ROOT_NMS", cyng::TC_NULL, ""}},
		{ smf::OBIS_NMS_ADDRESS, {"NMS_ADDRESS", cyng::TC_NULL, ""}},
		{ smf::OBIS_NMS_PORT, {"NMS_PORT", cyng::TC_NULL, ""}},
		{ smf::OBIS_NMS_USER, {"NMS_USER", cyng::TC_NULL, ""}},
		{ smf::OBIS_NMS_PWD, {"NMS_PWD", cyng::TC_NULL, ""}},
		{ smf::OBIS_NMS_ENABLED, {"NMS_ENABLED", cyng::TC_NULL, ""}},

		{ smf::OBIS_ROOT_REDIRECTOR, {"ROOT_REDIRECTOR", cyng::TC_NULL, ""}},
		{ smf::OBIS_REDIRECTOR_LOGIN, {"REDIRECTOR_LOGIN", cyng::TC_NULL, ""}},
		{ smf::OBIS_REDIRECTOR_ADDRESS, {"REDIRECTOR_ADDRESS", cyng::TC_NULL, "ip address"}},
		{ smf::OBIS_REDIRECTOR_SERVICE, {"REDIRECTOR_SERVICE", cyng::TC_UINT16, "port"}},
		{ smf::OBIS_REDIRECTOR_USER, {"REDIRECTOR_USER", cyng::TC_NULL, ""}},
		{ smf::OBIS_REDIRECTOR_PWD, {"REDIRECTOR_PWD", cyng::TC_NULL, ""}},
		//{ smf::OBIS_REDIRECTOR_ENABLED, {"REDIRECTOR_ENABLED", cyng::TC_NULL, ""}},


		//
		//	list types
		//
		{ smf::OBIS_LIST_CURRENT_DATA_RECORD, {"LIST_CURRENT_DATA_RECORD", cyng::TC_NULL, "current data set"}},	//	
		{ smf::OBIS_LIST_SERVICES, {"LIST_SERVICES", cyng::TC_NULL, ""}},
		{ smf::OBIS_FTP_UPDATE, {"FTP_UPDATE", cyng::TC_NULL, ""}}
	};

	std::ofstream ofs(__OUTPUT_PATH, std::ios::trunc);
	if (ofs.is_open()) {

		ofs
			<< "\t// "
			<< obis_map.size()
			<< " OBIS codes ("
			<< (obis_map.size() * 6u)
			<< " Bytes)"
			<< std::endl
			<< std::endl
			;

		ofs << std::setw(2)
			<< std::setfill('0')
			<< std::hex
			;


		std::size_t counter{ 0 };
		unsigned group = std::numeric_limits<unsigned>::max();
		for (auto const& ctx : obis_map) {

			++counter;
			std::cout 
				<< counter
				<< " - "
				<< cyng::traits::names[ctx.second.get_type()]
				<< " - "
				<< ctx.second.get_name()
				<< " "
				<< ctx.second.get_desc()
				<< std::endl;

			auto const next_group = static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_MEDIUM]);
			if (group != next_group) {

				ofs 
					<< "\t// #"
					<< std::dec
					<< counter
					<< std::hex
					<< std::endl;

				switch (next_group) {
				case cyng::obis::media::MEDIA_ABSTRACT:
					ofs << "\t// Abstract objects" << std::endl;
					break;
				case cyng::obis::media::MEDIA_ELECTRICITY:
					ofs << "\t// Electricity" << std::endl;
					break;
				case cyng::obis::media::MEDIA_HEAT_COST_ALLOCATOR:
					ofs << "\t// Heat Cost Allocators" << std::endl;
					break;
				case cyng::obis::media::MEDIA_COOLING:
					ofs << "\t// Cooling" << std::endl;
					break;
				case cyng::obis::media::MEDIA_HEAT:
					ofs << "\t// Heat" << std::endl;
					break;
				case cyng::obis::media::MEDIA_GAS:
					ofs << "\t// Gas" << std::endl;
					break;
				case cyng::obis::media::MEDIA_WATER_COLD:
					ofs << "\t// Water (cold)" << std::endl;
					break;
				case cyng::obis::media::MEDIA_WATER_HOT:
					ofs << "\t// Water (hot)" << std::endl;
					break;
				case 15: 
					ofs << "\t//  other" << std::endl;
					break;
				case 16: 
					ofs << "\t// Oil" << std::endl;
					break;
				case 17: 
					ofs << "\t// Compressed air" << std::endl;
					break;
				case 18:
					ofs << "\t// Nitrogen" << std::endl;
					break;

				default:
					ofs << "\t// next group" << std::endl;
					break;
				}
				group = next_group;
			}
			
			ofs
				<< "\tOBIS_CODE_DEFINITION("
				<< std::setw(2)
				<< next_group
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_CHANNEL])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_INDICATOR])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_MODE])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_QUANTITY])
				<< ", "
				<< std::setw(2)
				<< static_cast<unsigned>(ctx.first[cyng::obis::value_group::VG_STORAGE])
				<< ", "
				<< ctx.second.get_name()
				<< ");"
				;

			if (ctx.second.get_type() != cyng::TC_NULL) {
				ofs 
					<< "\t// "
					<< cyng::traits::names[ctx.second.get_type()];
			}
			if (!ctx.second.get_desc().empty()) {
				if (ctx.second.get_type() != cyng::TC_NULL) {
					ofs << " - ";
				}
				else {
					ofs	<< "\t// ";

				}
				ofs	<< ctx.second.get_desc();
			}
			ofs << std::endl;
		}

	}

	//std::cout << __OUTPUT_PATH << std::endl;
	return EXIT_SUCCESS;
}
