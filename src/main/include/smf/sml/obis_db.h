/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_OBIS_DB_H
#define NODE_SML_OBIS_DB_H

#include <smf/sml/intrinsics/obis.h>

//
//	macro to create OBIS codes without the 0x prefix
//
#define DEFINE_OBIS_CODE(p1, p2, p3, p4, p5, p6, name)	\
	OBIS_##name (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)


#define OBIS_CODE_DEFINITION(p1, p2, p3, p4, p5, p6, name)	\
	constexpr static obis OBIS_##name { 0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6 }; \
	constexpr static std::uint64_t CODE_##name { 0x##p1##p2##p3##p4##p5##p6 };


namespace node
{
	namespace sml
	{

		//
		//	general
		//
		OBIS_CODE_DEFINITION(00, 00, 00, 00, 00, FF, METER_ADDRESS)
		OBIS_CODE_DEFINITION(00, 00, 00, 00, 01, FF, IDENTITY_NR_1);
		OBIS_CODE_DEFINITION(00, 00, 00, 00, 02, FF, IDENTITY_NR_2);
		OBIS_CODE_DEFINITION(00, 00, 00, 00, 03, FF, IDENTITY_NR_3);
		OBIS_CODE_DEFINITION(00, 00, 00, 01, 00, FF, RESET_COUNTER);
		OBIS_CODE_DEFINITION(00, 00, 01, 00, 00, FF, REAL_TIME_CLOCK);	//	current time

		OBIS_CODE_DEFINITION(00, 00, 60, 01, 00, FF, SERIAL_NR);	//	Serial number I (assigned by the manufacturer).
		OBIS_CODE_DEFINITION(00, 00, 60, 01, 01, FF, SERIAL_NR_SECOND);	//	Serial number II (assigned by the manufacturer).
		OBIS_CODE_DEFINITION(00, 00, 60, 01, FF, FF, FABRICATION_NR);	//	fabrication number
		OBIS_CODE_DEFINITION(00, 00, 60, 02, 01, FF, DATE_TIME_PARAMETERISATION);	//	32 bit
		OBIS_CODE_DEFINITION(00, 00, 60, 07, 00, FF, POWER_OUTAGES);	//	Number of power failures 
		OBIS_CODE_DEFINITION(00, 00, 60, 08, 00, FF, SECONDS_INDEX);   //	[SML_Time] seconds index
		OBIS_CODE_DEFINITION(00, 00, 60, 10, 00, FF, LOGICAL_NAME);   //	
		OBIS_CODE_DEFINITION(00, 00, 60, F0, 0D, FF, HARDWARE_TYPE);   //	octet
		OBIS_CODE_DEFINITION(00, 00, 61, 61, 00, FF, MBUS_STATE);   //	Status according to EN13757-3 (error register)

		OBIS_CODE_DEFINITION(00, 80, 80, 00, 00, FF, STORAGE_TIME_SHIFT);	//	root push operations
		OBIS_CODE_DEFINITION(00, 80, 80, 00, 03, 01, HAS_SSL_CONFIG);		//	SSL/TSL configuration available
		OBIS_CODE_DEFINITION(00, 80, 80, 00, 04, FF, SSL_CERTIFICATES);		//	list of SSL certificates

		OBIS_CODE_DEFINITION(00, 80, 80, 01, 00, FF, ROOT_SECURITY);		//	list some basic security information
		OBIS_CODE_DEFINITION(00, 80, 80, 01, 01, FF, SECURITY_SERVER_ID);
		OBIS_CODE_DEFINITION(00, 80, 80, 01, 02, FF, SECURITY_OWNER);
		OBIS_CODE_DEFINITION(00, 80, 80, 01, 05, FF, SECURITY_05);	//	3
		OBIS_CODE_DEFINITION(00, 80, 80, 01, 06, FF, SECURITY_06);	//	3
		OBIS_CODE_DEFINITION(00, 80, 80, 01, 06, 10, SECURITY_06_10);	//	3
		OBIS_CODE_DEFINITION(00, 80, 80, 01, 07, FF, SECURITY_07);	//	1

		OBIS_CODE_DEFINITION(00, 80, 80, 10, 00, 01, CLASS_OP_LSM_STATUS);	//	LSM status
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 00, FF, ACTUATORS);	//	list of actuators
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 10, FF, CLASS_OP_LSM_ACTOR_ID);	//	ServerID des Aktors (uint16)
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 11, FF, CLASS_OP_LSM_CONNECT);	//	Verbindungsstatus
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 11, 01, CLASS_OP_LSM_SWITCH);	//	Schaltanforderung
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 13, FF, CLASS_OP_LSM_FEEDBACK);	//	feedback configuration
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 14, FF, CLASS_OP_LSM_LOAD);	//	number of loads
		OBIS_CODE_DEFINITION(00, 80, 80, 11, 15, FF, CLASS_OP_LSM_POWER);	//	total power
		OBIS_CODE_DEFINITION(00, 80, 80, 11, A0, FF, CLASS_STATUS);	//	see: 2.2.1.3 Status der Aktoren (Kontakte)
		OBIS_CODE_DEFINITION(00, 80, 80, 11, A1, FF, CLASS_OP_LSM_VERSION);	//	LSM version
		OBIS_CODE_DEFINITION(00, 80, 80, 11, A2, FF, CLASS_OP_LSM_TYPE);	//	LSM type
		OBIS_CODE_DEFINITION(00, 80, 80, 11, A9, 01, CLASS_OP_LSM_ACTIVE_RULESET); //	active ruleset
		OBIS_CODE_DEFINITION(00, 80, 80, 11, A9, 01, CLASS_RADIO_KEY);	//	or radio key
		OBIS_CODE_DEFINITION(00, 80, 80, 11, A9, 02, CLASS_OP_LSM_PASSIVE_RULESET);
		OBIS_CODE_DEFINITION(00, 80, 80, 14, 03, FF, CLASS_OP_LSM_JOB);//	Schaltauftrags ID ((octet string)	
		OBIS_CODE_DEFINITION(00, 80, 80, 14, 20, FF, CLASS_OP_LSM_POSITION);	//	current position

		OBIS_CODE_DEFINITION(00, B0, 00, 02, 00, 00, CLASS_MBUS);
		OBIS_CODE_DEFINITION(00, B0, 00, 02, 00, 01, CLASS_MBUS_RO_INTERVAL);	//	readout interval in seconds % 3600 (33 36 30 30)
		OBIS_CODE_DEFINITION(00, B0, 00, 02, 00, 02, CLASS_MBUS_SEARCH_INTERVAL);	//	search interval in seconds % 0 (30)
		OBIS_CODE_DEFINITION(00, B0, 00, 02, 00, 03, CLASS_MBUS_SEARCH_DEVICE);	//	search device now and by restart	% True(54 72 75 65)
		OBIS_CODE_DEFINITION(00, B0, 00, 02, 00, 04, CLASS_MBUS_AUTO_ACTICATE);	//	automatic activation of meters     % False(46 61 6C 73 65)
		OBIS_CODE_DEFINITION(00, B0, 00, 02, 00, 05, CLASS_MBUS_BITRATE);		//	used baud rates(bitmap) % 82 (38 32)

		//	Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
		OBIS_CODE_DEFINITION(01, 00, 00, 00, 00, FF, SERVER_ID_1_1);
		OBIS_CODE_DEFINITION(01, 00, 00, 00, 01, FF, SERVER_ID_1_2);		//	Identifikationsnummer 1.2
		OBIS_CODE_DEFINITION(01, 00, 00, 00, 02, FF, SERVER_ID_1_3);		//	Identifikationsnummer 1.3
		OBIS_CODE_DEFINITION(01, 00, 00, 00, 03, FF, SERVER_ID_1_4);		//	Identifikationsnummer 1.4
		OBIS_CODE_DEFINITION(01, 00, 00, 00, 09, FF, DEVICE_ID);	//	encode profiles
		OBIS_CODE_DEFINITION(01, 00, 00, 02, 00, FF, SOFTWARE_ID);	//	octet

		OBIS_CODE_DEFINITION(01, 00, 00, 09, 0B, 00, CURRENT_UTC);	//	readout time in UTC

		OBIS_CODE_DEFINITION(81, 48, 27, 32, 06, 01, TCP_WAIT_TO_RECONNECT);	//	u8
		OBIS_CODE_DEFINITION(81, 48, 31, 32, 02, 01, TCP_CONNECT_RETRIES);		//	u32

		OBIS_CODE_DEFINITION(81, 49, 00, 00, 10, FF, PUSH_SERVICE);	//	options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX


		/**
		 * Logbook interval [u16]. With value 0 logging is disabled. -1 is delete the logbook
		 */
		OBIS_CODE_DEFINITION(81, 81, 27, 32, 07, 01, OBISLOG_INTERVAL); 

		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 21, FF, PUSH_SERVICE_IPT); //	SML data response without request - typical IP - T push
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 22, FF, PUSH_SERVICE_SML); //	SML data response without request - target is SML client address
		//OBIS_CODE_DEFINITION(81, 81, C7, 8A, 23, FF, PUSH_SERVICE_KNX); //	target is KNX ID
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 23, FF, DATA_COLLECTOR_OBIS); //	OBIS list (data mirror)


		//
		//	sensor/meter parameters
		//

		OBIS_CODE_DEFINITION(81, 81, 61, 3C, 01, FF, DATA_USER_NAME);
		OBIS_CODE_DEFINITION(81, 81, 61, 3C, 02, FF, DATA_USER_PWD);


		OBIS_CODE_DEFINITION(81, 81, C7, 83, 82, 05, CLEAR_DATA_COLLECTOR);
		OBIS_CODE_DEFINITION(81, 81, C7, 83, 82, 0E, SET_ACTIVATE_FW);

		OBIS_CODE_DEFINITION(81, 81, C7, 86, 03, FF, DATA_AES_KEY);

		//
		//	Profiles (Lastgänge)
		//	The OBIS code to encode profiles is 81 81 C7 8A 83 FF
		//

		OBIS_CODE_DEFINITION(81, 81, C7, 86, 10, FF, PROFILE_1_MINUTE);	//	1 minute
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 11, FF, PROFILE_15_MINUTE);
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 12, FF, PROFILE_60_MINUTE);
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 13, FF, PROFILE_24_HOUR);
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 14, FF, PROFILE_LAST_2_HOURS);	//	past two hours
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 15, FF, PROFILE_LAST_WEEK);	//	weekly (on day change from Sunday to Monday)
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 16, FF, PROFILE_1_MONTH);	//	monthly recorded meter readings
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 17, FF, PROFILE_1_YEAR);	//	annually recorded meter readings
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 18, FF, PROFILE_INITIAL);	//	81, 81, C7, 86, 18, NN with NN = 01 .. 0A for open registration periods

		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 01, FF, ROOT_PUSH_OPERATIONS);	//	push root element 

		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 02, FF, PUSH_INTERVAL);	//	in seconds
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 03, FF, PUSH_DELAY);	//	in seconds

		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 04, FF, PUSH_SOURCE);	//	options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 42, FF, PUSH_SOURCE_PROFILE);	//!< new meter/sensor data
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 43, FF, PUSH_SOURCE_INSTALL);	//!< configuration changed
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 44, FF, PUSH_SOURCE_VISIBLE_SENSORS);	//!< list of visible meters changed
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 45, FF, PUSH_SOURCE_ACTIVE_SENSORS);	//!< list of active meters changed

		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 81, FF, PUSH_SERVER_ID);
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 82, FF, PUSH_IDENTIFIERS);	//	list of identifiers of the values to be delivered by the push source
		OBIS_CODE_DEFINITION(81, 81, C7, 8A, 83, FF, PROFILE);	//	encode profiles

		/**
		 * @return true id the 4th first bytes are 0x81, 0x81, 0xC7, 0x86
		 */
		bool is_profile(obis const&);

		//
		//	root elements
		//81, 81, C7, 81, 0E, FF 
		OBIS_CODE_DEFINITION(00, 80, 80, 00, 10, FF, ROOT_MEMORY_USAGE);	//	request memory usage
		OBIS_CODE_DEFINITION(00, 80, 80, 00, 11, FF, ROOT_MEMORY_MIRROR);	
		OBIS_CODE_DEFINITION(00, 80, 80, 00, 12, FF, ROOT_MEMORY_TMP);

		OBIS_CODE_DEFINITION(81, 02, 00, 07, 00, FF, ROOT_CUSTOM_INTERFACE);	//	see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle
		OBIS_CODE_DEFINITION(81, 02, 00, 07, 10, FF, ROOT_CUSTOM_PARAM);	//	see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle 
		OBIS_CODE_DEFINITION(81, 04, 00, 06, 00, FF, ROOT_WAN);	//	see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status 
		OBIS_CODE_DEFINITION(81, 04, 00, 07, 00, FF, ROOT_WAN_PARAM);	//	see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter 
		OBIS_CODE_DEFINITION(81, 04, 02, 07, 00, FF, ROOT_GSM);	//	see: Datenstruktur zum Lesen/Setzen der GSM Parameter 
		OBIS_CODE_DEFINITION(81, 04, 0D, 07, 00, FF, ROOT_GPRS_PARAM);	//	see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter 
		OBIS_CODE_DEFINITION(81, 04, 0D, 08, 00, FF, ROOT_GSM_STATUS);
		OBIS_CODE_DEFINITION(81, 04, 0E, 06, 00, FF, PLC_STATUS);
		//OBIS_CODE_DEFINITION(81, 04, 0E, 06, 00, FF, PLC_STATUS);

		//OBIS_CODE_DEFINITION(81, 06, 00, 00, 01, 00, W_MBUS_STATUS_MANUFACTURER);	//	[string] manufacturer of wireless M-Bus adapter
		//OBIS_CODE_DEFINITION(81, 06, 00, 00, 03, 00, W_MBUS_STATUS_ID);	//	[string] adapter ID - see EN 13757-3/4
		//OBIS_CODE_DEFINITION(81, 06, 00, 02, 00, 00, W_MBUS_STATUS_FIRMWARE);	//	[string] adapter firmware version
		//OBIS_CODE_DEFINITION(81, 06, 00, 02, 03, 00, W_MBUS_STATUS_HARDWARE);	//	[string] adapter hardware version

		OBIS_CODE_DEFINITION(81, 06, 0F, 06, 00, FF, ROOT_W_MBUS_STATUS);	//	see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
		OBIS_CODE_DEFINITION(81, 46, 00, 00, 02, FF, PROFILE_ADDRESS);
		OBIS_CODE_DEFINITION(81, 47, 17, 07, 00, FF, PUSH_TARGET);	//	push target name
		OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 00, COMPUTER_NAME);	
		OBIS_CODE_DEFINITION(81, 48, 00, 32, 02, 01, LAN_DHCP_ENABLED);	//	[bool]
		OBIS_CODE_DEFINITION(81, 48, 0D, 06, 00, FF, ROOT_LAN_DSL);	//	see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter
		OBIS_CODE_DEFINITION(81, 49, 0D, 06, 00, FF, ROOT_IPT_STATE);	//	see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status 
		OBIS_CODE_DEFINITION(81, 49, 0D, 07, 00, FF, ROOT_IPT_PARAM);	//	see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter 

		//	ip-t status
		//OBIS_CODE_DEFINITION(81, 49, 17, 07, 00, NN, ROOT_IPT_STATE_ADDRESS);	//	IP adress
		//OBIS_CODE_DEFINITION(81, 49, 1A, 07, 00, NN, ROOT_IPT_STATE_PORT_LOCAL);	//	local port
		//OBIS_CODE_DEFINITION(81, 49, 19, 07, 00, NN, ROOT_IPT_STATE_PORT_REMOTE);	//	remote port

		OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 01, PEER_OBISLOG);	//	peer address: OBISLOG
		OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 02, PEER_SCM);	//	peer address: SCM (power)
		OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 11, PEER_USERIF);	//	peer address: USERIF
		OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, 13, PEER_ADDRESS_WANGSM);	//	peer address: WAN/GSM
		OBIS_CODE_DEFINITION(81, 81, 00, 00, 00, FF, PEER_ADDRESS);	//	unit is 255

		OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 00, VERSION);		//	[string]
		OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 02, FILE_NAME);	//	[string]
		OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 03, MSG_COUNTER);	//	[u32] Anzahl aller Nachrichten zur Übertragung des Binary
		OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 04, LAST_MSG);	//	[u32] Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary
		OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 05, MSG_NUMBER);
		OBIS_CODE_DEFINITION(81, 81, 00, 02, 02, FF, BLOCK_NUMBER);
		OBIS_CODE_DEFINITION(81, 81, 00, 02, 03, FF, BINARY_DATA);

		OBIS_CODE_DEFINITION(81, 81, 01, 16, FF, FF, ROOT_NEW_DEVICES);	//	new active devices
		OBIS_CODE_DEFINITION(81, 81, 10, 26, FF, FF, ROOT_INVISIBLE_DEVICES);	//	not longer visible devices
		OBIS_CODE_DEFINITION(81, 81, 10, 06, FF, FF, ROOT_VISIBLE_DEVICES);	//	visible devices (Liste der sichtbaren Sensoren)
		OBIS_CODE_DEFINITION(81, 81, 10, 06, 01, FF, CODE_LIST_1_VISIBLE_DEVICES);	//	1. Liste der sichtbaren Sensoren
		OBIS_CODE_DEFINITION(81, 81, 11, 06, FF, FF, ROOT_ACTIVE_DEVICES);	//	active devices (Liste der aktiven Sensoren)
		OBIS_CODE_DEFINITION(81, 81, 11, 06, 01, FF, CODE_LIST_1_ACTIVE_DEVICES);	//	1. Liste der aktiven Sensoren)
		OBIS_CODE_DEFINITION(81, 81, 11, 06, FB, FF, ACTIVATE_DEVICE);	//	activate meter/sensor
		OBIS_CODE_DEFINITION(81, 81, 11, 06, FC, FF, DEACTIVATE_DEVICE);	//	deactivate meter/sensor
		OBIS_CODE_DEFINITION(81, 81, 11, 06, FD, FF, DELETE_DEVICE);	//	delete meter/sensor
		OBIS_CODE_DEFINITION(81, 81, 12, 06, FF, FF, ROOT_DEVICE_INFO);	//	extended device information
		OBIS_CODE_DEFINITION(81, 81, 81, 60, FF, FF, ROOT_ACCESS_RIGHTS);	//	see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte 

		OBIS_CODE_DEFINITION(81, 81, 81, 61, FF, FF, ACCESS_USER_NAME);	//	user name for access
		OBIS_CODE_DEFINITION(81, 81, 81, 62, FF, FF, ACCESS_PASSWORD);	//	SHA256 encrypted
		OBIS_CODE_DEFINITION(81, 81, 81, 63, FF, FF, ACCESS_PUBLIC_KEY);

		OBIS_CODE_DEFINITION(81, 81, C7, 81, 01, FF, ROOT_FILE_TRANSFER);	//	7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) 
		OBIS_CODE_DEFINITION(81, 81, C7, 81, 0E, FF, DATA_FIRMWARE);
		OBIS_CODE_DEFINITION(81, 81, C7, 81, 0C, FF, DATA_FILENAME);
		OBIS_CODE_DEFINITION(81, 81, C7, 81, 0F, FF, DATA_FILENAME_INDIRECT);
		OBIS_CODE_DEFINITION(81, 81, C7, 81, 0D, FF, DATA_APPLICATION);
		OBIS_CODE_DEFINITION(81, 81, C7, 81, 10, FF, DATA_APPLICATION_INDIRECT);
		OBIS_CODE_DEFINITION(81, 81, C7, 81, 23, FF, DATA_PUSH_DETAILS);	// string

		OBIS_CODE_DEFINITION(81, 81, C7, 82, 81, FF, DATA_IP_ADDRESS);

		OBIS_CODE_DEFINITION(81, 81, C7, 82, 01, FF, ROOT_DEVICE_IDENT);	//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) 
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 02, FF, DEVICE_CLASS);	//	Geräteklasse (OBIS code or '2D 2D 2D')
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 03, FF, DATA_MANUFACTURER);	//	[string] FLAG
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 04, FF, SERVER_ID);		//	Server ID 
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 05, FF, DATA_PUBLIC_KEY);
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 06, FF, ROOT_FIRMWARE);	//	Firmware
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 09, FF, HARDWARE_FEATURES);	//	hardware equipment (charge, type, ...) 81 81 C7 82 0A NN

		OBIS_CODE_DEFINITION(81, 81, C7, 82, 08, FF, DEVICE_KERNEL);	//	[string]
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 0E, FF, DEVICE_ACTIVATED);

		//	device classes
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 41, FF, DEV_CLASS_BASIC_DIRECT);	//	3 x 230 /400 V and 5 (100) A 
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 42, FF, DEV_CLASS_BASIC_SEMI);	//	3 x 230 /400 V and 1 (6) A
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 43, FF, DEV_CLASS_BASIC_INDIRECT);	//	3 x  58 / 100 V and 1 (6) A 
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 45, FF, DEV_CLASS_IW);		//	IW module
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 46, FF, DEV_CLASS_PSTN);	//	PSTN/GPRS
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 47, FF, DEV_CLASS_GPRS);	//	GPRS/PLC
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 48, FF, DEV_CLASS_KM);		//	KM module (LAN/DSL)
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 49, FF, DEV_CLASS_NK);		//	NK/HS
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 4A, FF, DEV_CLASS_EXTERN);	//	external load profile collector 
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 4B, FF, DEV_CLASS_GW);		//	gateway
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 4F, FF, DEV_CLASS_LAN);	//	see DEV_CLASS_MUC_LAN
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 50, FF, DEV_CLASS_eHZ);
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 52, FF, DEV_CLASS_3HZ);
		OBIS_CODE_DEFINITION(81, 81, C7, 82, 53, FF, DEV_CLASS_MUC_LAN);	// (MUC-LAN/DSL)
		
		OBIS_CODE_DEFINITION(81, 81, C7, 83, 82, 01, REBOOT);	//	request reboot
		OBIS_CODE_DEFINITION(81, 81, C7, 83, 82, 07, UPDATE_FW);	//	activate firmware

		OBIS_CODE_DEFINITION(81, 81, C7, 86, 00, FF, ROOT_SENSOR_PARAMS);	//	data mirror root element (Eigenschaften eines Datenspiegels)
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 01, FF, ROOT_SENSOR_BITMASK);	//	[u16] Bitmask to define bits that will be transferred into log
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 02, FF, AVERAGE_TIME_MS);	//	average time between two received data records (milliseconds)
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 20, FF, ROOT_DATA_COLLECTOR);	//	data collector root element (Eigenschaften eines Datensammlers)
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 21, FF, DATA_COLLECTOR_ACTIVE);	//	true/false
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 22, FF, DATA_COLLECTOR_SIZE);		//	max. table size
		OBIS_CODE_DEFINITION(81, 81, C7, 86, 04, FF, TIME_REFERENCE);	//	[u8] 0 == UTC, 1 == UTC + time zone, 2 == local time
		OBIS_CODE_DEFINITION(81, 81, C7, 87, 81, FF, DATA_REGISTER_PERIOD);	//	 (u32) register period in seconds (0 == event driven)
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 01, FF, ROOT_NTP);	//	NTP configuration
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 02, FF, CODE_NTP_SERVER);	//	List of NTP servers
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 03, FF, CODE_NTP_PORT);	//	[u16] NTP port (123)
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 04, FF, CODE_NTP_TZ);		//	[u32] timezone
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 05, FF, CODE_NTP_OFFSET);	//	[sec] Offset to transmission of the signal for synchronization
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 06, FF, CODE_NTP_ACTIVE);	//	[bool] NTP enabled/disables
		OBIS_CODE_DEFINITION(81, 81, C7, 88, 10, FF, ROOT_DEVICE_TIME);	//	device time

		OBIS_CODE_DEFINITION(81, 81, C7, 90, 00, FF, ROOT_IF);	 //	root of all interfaces
		//OBIS_CODE_DEFINITION(81, 81, C7, 91, 00, FF, ROOT_IF_?);	
		//OBIS_CODE_DEFINITION(81, 81, C7, 92, 00, FF, ROOT_IF_?);	

		OBIS_CODE_DEFINITION(81, 81, C7, 93, 00, FF, IF_1107);	 //	1107 interface (IEC 62056-21)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 01, FF, IF_1107_ACTIVE); //	(bool) - if true 1107 interface active otherwise SML interface active
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 02, FF, IF_1107_LOOP_TIME); //	(u) - Loop timeout in seconds
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 03, FF, IF_1107_RETRIES); //	(u) - Retry count
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 04, FF, IF_1107_MIN_TIMEOUT); //	(u) - Minimal answer timeout(300)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 05, FF, IF_1107_MAX_TIMEOUT); //	(u) - Maximal answer timeout(5000)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 06, FF, IF_1107_MAX_DATA_RATE); //	(u) - Maximum data bytes(10240)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 07, FF, IF_1107_RS485); //	(bool) - if true RS 485, otherwise RS 323
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 08, FF, IF_1107_PROTOCOL_MODE); //	(u) - Protocol mode(A ... D)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 09, FF, IF_1107_METER_LIST); // Liste der abzufragenden 1107 Zähler
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 10, FF, IF_1107_AUTO_ACTIVATION); //(True)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 11, FF, IF_1107_TIME_GRID); //	time grid of load profile readout in seconds
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 13, FF, IF_1107_TIME_SYNC); //	time sync in seconds
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 14, FF, IF_1107_MAX_VARIATION); //(seconds)

		OBIS_CODE_DEFINITION(81, 81, C7, 93, 0A, FF, IF_1107_METER_ID); //	(string)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 0B, FF, IF_1107_BAUDRATE); //	(u)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 0C, FF, IF_1107_ADDRESS); //	(string)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 0D, FF, IF_1107_P1); //	(string)
		OBIS_CODE_DEFINITION(81, 81, C7, 93, 0E, FF, IF_1107_W5); //	(string)

																								  //
		//	Interfaces
		//
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 00, FF, IF_LAN_DSL);	// see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 00, 00, CODE_IF_LAN_ADDRESS);	// IPv4 or IPv6 address
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 01, 00, CODE_IF_LAN_SUBNET_MASK);	// IPv4 or IPv6
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 02, 00, CODE_IF_LAN_GATEWAY);	// IPv4 or IPv6
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 04, 00, CODE_IF_LAN_DNS_PRIMARY);	// IPv4 or IPv6
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 05, 00, CODE_IF_LAN_DNS_SECONDARY);	// IPv4 or IPv6
		OBIS_CODE_DEFINITION(81, 48, 17, 07, 06, 00, CODE_IF_LAN_DNS_TERTIARY);	// IPv4 or IPv6

		//OBIS_CODE_DEFINITION(81, 04, 02, 07, 00, FF, CODE_IF_GSM);
		//OBIS_CODE_DEFINITION(81, 04, 0D, 07, 00, FF, CODE_IF_GPRS);
		OBIS_CODE_DEFINITION(81, 02, 00, 07, 00, FF, CODE_IF_USER);	//	Endkundenschnittstelle:
		//OBIS_CODE_DEFINITION(81, 49, 0D, 07, 00, FF, CODE_IF_IPT);	//	 same as OBIS_ROOT_IPT_PARAM
		OBIS_CODE_DEFINITION(81, 05, 0D, 07, 00, FF, IF_EDL);		//	M-Bus EDL (RJ10)

		OBIS_CODE_DEFINITION(81, 05, 0D, 07, 00, 01, IF_EDL_PROTOCOL);	//	[u8] always 1
		OBIS_CODE_DEFINITION(81, 05, 0D, 07, 00, 02, IF_EDL_BAUDRATE);	//	0 = auto, 6 = 9600, 10 = 115200 baud

		OBIS_CODE_DEFINITION(81, 06, 19, 07, 00, FF, IF_wMBUS);	//	Wireless M-BUS:
		OBIS_CODE_DEFINITION(81, 04, 18, 07, 00, FF, IF_PLC);
		//OBIS_CODE_DEFINITION(81, 05, 0D, 07, 00, FF, CODE_IF_SyM2);	//	same as IF_EDL
		OBIS_CODE_DEFINITION(81, 00, 00, 09, 0B, 00, ACT_SENSOR_TIME);	//	[SML_Time] actSensorTime - current UTC time
		OBIS_CODE_DEFINITION(81, 00, 00, 09, 0B, 01, TZ_OFFSET);	//	[u16] offset to actual time zone in minutes (-720 .. +720)

		OBIS_CODE_DEFINITION(81, 81, C7, 89, E1, FF, CLASS_OP_LOG);
		OBIS_CODE_DEFINITION(81, 81, C7, 89, E2, FF, CLASS_EVENT);	//	Ereignis (uint32)

		OBIS_CODE_DEFINITION(81, 01, 00, 00, 01, 00, INTERFACE_01_NAME);	//	[string] interface name
		OBIS_CODE_DEFINITION(81, 02, 00, 00, 01, 00, INTERFACE_02_NAME);	//	[string] interface name
		OBIS_CODE_DEFINITION(81, 03, 00, 00, 01, 00, INTERFACE_03_NAME);	//	[string] interface name
		OBIS_CODE_DEFINITION(81, 04, 00, 00, 01, 00, INTERFACE_04_NAME);	//	[string] interface name

																					//
		//	wMBus - 81 06 0F 06 00 FF
		//	7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
		//
		OBIS_CODE_DEFINITION(81, 06, 00, 00, 01, 00, W_MBUS_ADAPTER_MANUFACTURER);	//	[string] 
		OBIS_CODE_DEFINITION(81, 06, 00, 00, 03, 00, W_MBUS_ADAPTER_ID);
		OBIS_CODE_DEFINITION(81, 06, 00, 02, 00, 00, W_MBUS_FIRMWARE);	//	[string]
		OBIS_CODE_DEFINITION(81, 06, 00, 02, 03, FF, W_MBUS_HARDWARE);	//	[string]
		OBIS_CODE_DEFINITION(81, 06, 00, 02, 04, FF, W_MBUS_FIELD_STRENGTH);	//	[u16] dbm
		OBIS_CODE_DEFINITION(81, 06, 00, 03, 74, FF, W_MBUS_LAST_RECEPTION);	//	[seconds] Time since last radio telegram reception

		//
		//	Wireless M-BUS config
		//	81 06 19 07 00 FF, IF_wMBUS
		//
		OBIS_CODE_DEFINITION(81, 06, 19, 07, 01, FF, W_MBUS_PROTOCOL);	
		OBIS_CODE_DEFINITION(81, 06, 19, 07, 02, FF, W_MBUS_MODE_S);	//	u8
		OBIS_CODE_DEFINITION(81, 06, 19, 07, 03, FF, W_MBUS_MODE_T);	//	u8
		//OBIS_CODE_DEFINITION(81, 06, 19, 07, 04, FF, W_MBUS_MODE_C);	//	alternating
		//OBIS_CODE_DEFINITION(81, 06, 19, 07, 05, FF, W_MBUS_MODE_N);	//	parallel
		//OBIS_CODE_DEFINITION(81, 06, 19, 07, 06, FF, W_MBUS_MODE_S_SYN);	//	Used for synchronized messages in meters

		OBIS_CODE_DEFINITION(81, 06, 27, 32, 03, 01, W_MBUS_REBOOT);	//	u32 (seconds)
		OBIS_CODE_DEFINITION(81, 06, 19, 07, 04, FF, W_MBUS_POWER);		//	note: same code as W_MBUS_MODE_C
		OBIS_CODE_DEFINITION(81, 06, 19, 07, 11, FF, W_MBUS_INSTALL_MODE);

		//	Spannung - voltage
		//	Strom - current
		//	Leistungsfaktor - power factor
		//	Scheinleistung - apparent power
		//	Wirkleistung - effective (or active) power

		OBIS_CODE_DEFINITION(01, 00, 01, 08, 00, FF, REG_POS_AE_NO_TARIFF);	//	1.8.0 - Zählwerk pos. Wirkenergie, 	tariflos
		OBIS_CODE_DEFINITION(01, 00, 01, 08, 01, FF, REG_POS_AE_T1);		//	1.8.1 - Zählwerk pos. Wirkenergie, Tarif 1
		OBIS_CODE_DEFINITION(01, 00, 01, 08, 02, FF, REG_POS_AE_T2);		//	1.8.2 - Zählwerk pos. Wirkenergie, Tarif 2
		OBIS_CODE_DEFINITION(01, 00, 02, 08, 00, FF, REG_NEG_AE_NO_TARIFF);	//	2.8.0 - Zählwerk neg. Wirkenergie, 	tariflos
		OBIS_CODE_DEFINITION(01, 00, 02, 08, 01, FF, REG_NEG_AE_T1);		//	2.8.1 - Zählwerk neg. Wirkenergie, Tarif 1
		OBIS_CODE_DEFINITION(01, 00, 02, 08, 02, FF, REG_NEG_AE_T2);		//	2.8.2 - Zählwerk neg. Wirkenergie, Tarif 2
		OBIS_CODE_DEFINITION(01, 00, 0F, 07, 00, FF, REG_CUR_POS_AE);		//	15.7.0 - Aktuelle positive Wirkleistung, Betrag
		OBIS_CODE_DEFINITION(01, 00, 10, 07, 00, ff, REG_CUR_AP);		//	aktuelle Wirkleistung 

		//	01 00 24 07 00 FF: Wirkleistung L1
		//01 00 38 07 00 FF: Wirkleistung L2
		//01 00 4C 07 00 FF: Wirkleistung L3
		//01 00 60 32 00 02: Aktuelle Chiptemperatur
		//01 00 60 32 00 03: Minimale Chiptemperatur
		//01 00 60 32 00 04: Maximale Chiptemperatur
		//01 00 60 32 00 05: Gemittelte Chiptemperatur
		OBIS_CODE_DEFINITION(01, 00, 1F, 07, 00, FF, REG_CURRENT_L1);	//	Strom L1
		OBIS_CODE_DEFINITION(01, 00, 20, 07, 00, FF, REG_VOLTAGE_L1);	//	Spannung L1
		OBIS_CODE_DEFINITION(01, 00, 33, 07, 00, FF, REG_CURRENT_L2);	//	Strom L2
		OBIS_CODE_DEFINITION(01, 00, 34, 07, 00, FF, REG_VOLTAGE_L2);	//	Spannung L2
		OBIS_CODE_DEFINITION(01, 00, 47, 07, 00, FF, REG_CURRENT_L3);	//	Strom L3
		OBIS_CODE_DEFINITION(01, 00, 48, 07, 00, FF, REG_VOLTAGE_L3);	//	Spannung L3 
		OBIS_CODE_DEFINITION(01, 00, 60, 32, 03, 03, REG_VOLTAGE_MIN);	//	Spannungsminimum
		OBIS_CODE_DEFINITION(01, 00, 60, 32, 03, 04, REG_VOLTAGE_MAX);	//	Spannungsmaximum


		//		constexpr static obis	OBIS_CLASS_OP_LOG_EVENT(0x81, 0x81, 0xC7, 0x89, 0xE2, 0xFF);	//	Ereignis (uint32)
		constexpr static obis	OBIS_CLASS_OP_LOG_PEER_ADDRESS(0x81, 0x81, 0x00, 0x00, 0x00, 0xFF);	//	Peer Adresse (octet string)
		constexpr static obis	OBIS_CLASS_OP_MONITORING_STATUS(0x00, 0x80, 0x80, 0x11, 0x13, 0x01);	//	Überwachungsstatus (uint8) - tatsächlich i32

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
		OBIS_CODE_DEFINITION(81, 00, 60, 05, 00, 00, CLASS_OP_LOG_STATUS_WORD);	
		OBIS_CODE_DEFINITION(81, 04, 2B, 07, 00, 00, CLASS_OP_LOG_FIELD_STRENGTH);
		OBIS_CODE_DEFINITION(81, 04, 1A, 07, 00, 00, CLASS_OP_LOG_CELL);	//	aktuelle Zelleninformation (uint16)
		OBIS_CODE_DEFINITION(81, 04, 17, 07, 00, 00, CLASS_OP_LOG_AREA_CODE);	//	aktueller Location - oder Areacode(uint16)
		OBIS_CODE_DEFINITION(81, 04, 0D, 06, 00, 00, CLASS_OP_LOG_PROVIDER);	//	aktueller Provider-Identifier(uint32)

		OBIS_CODE_DEFINITION(81, 04, 0D, 06, 00, FF, GSM_ADMISSIBLE_OPERATOR);

		//
		//	attention codes
		//
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 00, ATTENTION_UNKNOWN_ERROR);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 01, ATTENTION_UNKNOWN_SML_ID);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 02, ATTENTION_NOT_AUTHORIZED);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 03, ATTENTION_NO_SERVER_ID);	//	unable to find recipient for request
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 04, ATTENTION_NO_REQ_FIELD);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 05, ATTENTION_CANNOT_WRITE);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 06, ATTENTION_CANNOT_READ);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 07, ATTENTION_COMM_ERROR);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 08, ATTENTION_PARSER_ERROR);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 09, ATTENTION_OUT_OF_RANGE);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 0A, ATTENTION_NOT_EXECUTED);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 0B, ATTENTION_INVALID_CRC);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 0C, ATTENTION_NO_BROADCAST);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 0D, ATTENTION_UNEXPECTED_MSG);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 0E, ATTENTION_UNKNOWN_OBIS_CODE);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 0F, ATTENTION_UNSUPPORTED_DATA_TYPE);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 10, ATTENTION_ELEMENT_NOT_OPTIONAL);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 11, ATTENTION_NO_ENTRIES);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 12, ATTENTION_END_LIMIT_BEFORE_START);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 13, ATTENTION_NO_ENTRIES_IN_RANGE);	//	range is empty - not the profile
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FE, 14, ATTENTION_MISSING_CLOSE_MSG);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FD, 00, ATTENTION_OK);
		OBIS_CODE_DEFINITION(81, 81, C7, C7, FD, 01, ATTENTION_JOB_IS_RUNNINNG);


		//
		//	list types
		//
		OBIS_CODE_DEFINITION(99, 00, 00, 00, 00, 03, LIST_CURRENT_DATA_RECORD);	//	current data set
		OBIS_CODE_DEFINITION(99, 00, 00, 00, 00, 04, LIST_SERVICES);
		OBIS_CODE_DEFINITION(99, 00, 00, 00, 00, 05, FTP_UPDATE);

		//
		//	source for log entries (OBIS-T)
		//
		OBIS_CODE_DEFINITION(81, 01, 00, 00, 00, FF, LOG_SOURCE_ETH_AUX);
		OBIS_CODE_DEFINITION(81, 02, 00, 00, 00, FF, LOG_SOURCE_ETH_CUSTOM);
		OBIS_CODE_DEFINITION(81, 03, 00, 00, 00, FF, LOG_SOURCE_RS232);
		OBIS_CODE_DEFINITION(81, 04, 00, 00, 00, FF, LOG_SOURCE_ETH);	//	WAN interface
		OBIS_CODE_DEFINITION(81, 05, 00, 00, 00, FF, LOG_SOURCE_eHZ);	
		OBIS_CODE_DEFINITION(81, 06, 00, 00, 00, FF, LOG_SOURCE_wMBUS);

		OBIS_CODE_DEFINITION(81, 41, 00, 00, 00, FF, LOG_SOURCE_IP);

		OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 01, LOG_SOURCE_SML_EXT);
		OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 02, LOG_SOURCE_SML_CUSTOM);
		OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 03, LOG_SOURCE_SML_SERVICE);
		OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 04, LOG_SOURCE_SML_WAN);
		OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 05, LOG_SOURCE_SML_eHZ);
		OBIS_CODE_DEFINITION(81, 42, 00, 00, 00, 06, LOG_SOURCE_SML_wMBUS);

		OBIS_CODE_DEFINITION(81, 45, 00, 00, 00, FF, LOG_SOURCE_PUSH_SML);
		OBIS_CODE_DEFINITION(81, 46, 00, 00, 00, FF, LOG_SOURCE_PUSH_IPT_SOURCE);
		OBIS_CODE_DEFINITION(81, 47, 00, 00, 00, FF, LOG_SOURCE_PUSH_IPT_SINK);
		OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 01, LOG_SOURCE_WAN_DHCP);
		OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 02, LOG_SOURCE_WAN_IP);
		OBIS_CODE_DEFINITION(81, 48, 00, 00, 00, 03, LOG_SOURCE_WAN_PPPoE);
		OBIS_CODE_DEFINITION(81, 49, 00, 00, 00, 01, LOG_SOURCE_WAN_IPT_CONTROLLER);
		OBIS_CODE_DEFINITION(81, 49, 00, 00, 00, 02, LOG_SOURCE_WAN_IPT);
		OBIS_CODE_DEFINITION(81, 4A, 00, 00, 00, FF, LOG_SOURCE_WAN_NTP);

		OBIS_CODE_DEFINITION(81, 81, 00, 02, 00, 01, SET_START_FW_UPDATE);
		OBIS_CODE_DEFINITION(81, 81, 00, 03, 00, 01, SET_DISPATCH_FW_UPDATE);
		

		OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 01, LOG_SOURCE_LOG);
		OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 02, LOG_SOURCE_SCM);	//	internal software function
		OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 03, LOG_SOURCE_UPDATE);
		OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 04, LOG_SOURCE_SMLC);

		OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 0C, LOG_SOURCE_LEDIO);

		OBIS_CODE_DEFINITION(81, 81, 10, 00, 00, 14, LOG_SOURCE_WANPLC);


		/**
		 * @return a short description of the OBIS code if available
		 */
		std::string get_name(obis const&);

		/**
		 * @return a short description of the OBIS code if available
		 */
		std::string get_attention_name(obis const&);

		/**
		 * @return name of the LSM event
		 */
		const char* get_LSM_event_name(std::uint32_t);

		/**
		 * @return profile name
		 */
		const char* get_profile_name(obis const&);

	}	//	sml
}	//	node


#endif	
