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

#define OBIS_CODE(p1, p2, p3, p4, p5, p6)	\
	node::sml::make_obis (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

namespace node
{
	namespace sml
	{

		//
		//	general
		//
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 00, 00, 00, FF, METER_ADDRESS);
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 00, 00, 01, FF, IDENTITY_NR_1);
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 00, 00, 02, FF, IDENTITY_NR_2);
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 00, 00, 03, FF, IDENTITY_NR_3);
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 00, 01, 00, FF, RESET_COUNTER);
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 01, 00, 00, FF, REAL_TIME_CLOCK);	//	current time

		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 01, 00, FF, SERIAL_NR);	//	Serial number I (assigned by the manufacturer).
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 01, 01, FF, SERIAL_NR_SECOND);	//	Serial number II (assigned by the manufacturer).
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 01, FF, FF, FABRICATION_NR);	//	fabrication number
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 02, 01, FF, DATE_TIME_PARAMETERISATION);	//	32 bit
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 07, 00, FF, POWER_OUTAGES);	//	Number of power failures 
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 08, 00, FF, CONFIG_OVERVIEW);   //	
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, 10, 00, FF, LOGICAL_NAME);   //	
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 60, F0, 0D, FF, HARDWARE_TYPE);   //	octet
		constexpr static obis	DEFINE_OBIS_CODE(00, 00, 61, 61, 00, FF, MBUS_STATE);   //	Status according to EN13757-3 (error register)

		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 00, 00, FF, STORAGE_TIME_SHIFT);	//	root push operations
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 10, 00, 01, CLASS_OP_LSM_STATUS);	//	LSM status
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 00, FF, ACTUATORS);	//	list of actuators
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 10, FF, CLASS_OP_LSM_ACTOR_ID);	//	ServerID des Aktors (uint16)
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 11, FF, CLASS_OP_LSM_CONNECT);	//	Verbindungsstatus
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 11, 01, CLASS_OP_LSM_SWITCH);	//	Schaltanforderung
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 13, FF, CLASS_OP_LSM_FEEDBACK);	//	feedback configuration
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 14, FF, CLASS_OP_LSM_LOAD);	//	number of loads
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 15, FF, CLASS_OP_LSM_POWER);	//	total power
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A0, FF, CLASS_STATUS);	//	see: 2.2.1.3 Status der Aktoren (Kontakte)
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A1, FF, CLASS_OP_LSM_VERSION);	//	LSM version
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A2, FF, CLASS_OP_LSM_TYPE);	//	LSM type
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A9, 01, CLASS_OP_LSM_ACTIVE_RULESET); //	active ruleset
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A9, 01, CLASS_RADIO_KEY);	//	or radio key
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A9, 02, CLASS_OP_LSM_PASSIVE_RULESET);
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 14, 03, FF, CLASS_OP_LSM_JOB);//	Schaltauftrags ID ((octet string)	
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 14, 20, FF, CLASS_OP_LSM_POSITION);	//	current position

		constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 00, CLASS_MBUS);
		constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 01, CLASS_MBUS_RO_INTERVAL);	//	readout interval in seconds % 3600 (33 36 30 30)
		constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 02, CLASS_MBUS_SEARCH_INTERVAL);	//	search interval in seconds % 0 (30)
		constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 03, CLASS_MBUS_SEARCH_DEVICE);	//	search device now and by restart	% True(54 72 75 65)
		constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 04, CLASS_MBUS_AUTO_ACTICATE);	//	automatic activation of meters     % False(46 61 6C 73 65)
		constexpr static obis	DEFINE_OBIS_CODE(00, B0, 00, 02, 00, 05, CLASS_MBUS_BITRATE);		//	used baud rates(bitmap) % 82 (38 32)

		//	Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 00, FF, SERVER_ID_1_1);
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 01, FF, SERVER_ID_1_2);		//	Identifikationsnummer 1.2
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 02, FF, SERVER_ID_1_3);		//	Identifikationsnummer 1.3
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 03, FF, SERVER_ID_1_4);		//	Identifikationsnummer 1.4
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 09, FF, DEVICE_ID);	//	encode profiles
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 02, 00, FF, SOFTWARE_ID);	//	octet

		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 00, 09, 0B, 00, CURRENT_UTC);	//	readout time in UTC

		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 27, 32, 06, 01, TCP_WAIT_TO_RECONNECT);	//	u8
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 31, 32, 02, 01, TCP_CONNECT_RETRIES);		//	u32

		constexpr static obis	DEFINE_OBIS_CODE(81, 49, 00, 00, 10, FF, PUSH_SERVICE);	//	options are PUSH_SERVICE_IPT, PUSH_SERVICE_SML or PUSH_SERVICE_KNX

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 21, FF, PUSH_SERVICE_IPT); //	SML data response without request - typical IP - T push
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 22, FF, PUSH_SERVICE_SML); //	SML data response without request - target is SML client address
		//constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 23, FF, PUSH_SERVICE_KNX); //	target is KNX ID
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 23, FF, DATA_COLLECTOR_OBIS); //	OBIS list (data mirror)


		//
		//	sensor/meter parameters
		//

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 61, 3C, 01, FF, DATA_USER_NAME);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 61, 3C, 02, FF, DATA_USER_PWD);


		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 83, 82, 0E, SET_ACTIVATE_FW);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 03, FF, DATA_AES_KEY);

		//
		//	Profiles (Lastgänge)
		//	The OBIS code to encode profiles is 81 81 C7 8A 83 FF
		//

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 10, FF, PROFILE_1_MINUTE);	//	1 minute
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 11, FF, PROFILE_15_MINUTE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 12, FF, PROFILE_60_MINUTE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 13, FF, PROFILE_24_HOUR);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 14, FF, PROFILE_LAST_2_HOURS);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 15, FF, PROFILE_LAST_WEEK);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 16, FF, PROFILE_1_MONTH);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 17, FF, PROFILE_1_YEAR);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 18, FF, PROFILE_INITIAL);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 01, FF, PUSH_OPERATIONS);	//	7.3.1.26 Datenstruktur zum Transport der Eigenschaften von Push-Vorgängen. 

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 02, FF, PUSH_INTERVAL);	//	in seconds
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 03, FF, PUSH_DELAY);	//	in seconds

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 04, FF, PUSH_SOURCE);	//	options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 42, FF, PUSH_SOURCE_PROFILE);	//!< new meter/sensor data
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 43, FF, PUSH_SOURCE_INSTALL);	//!< configuration changed
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 44, FF, PUSH_SOURCE_SENSOR_LIST);	//!< list of visible meters changed

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 81, FF, PUSH_SERVER_ID);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 83, FF, PROFILE);	//	encode profiles

		/**
		 * @return true id the 4th first bytes are 0x81, 0x81, 0xC7, 0x86
		 */
		bool is_profile(obis const&);

		//
		//	root elements
		//81, 81, C7, 81, 0E, FF 
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 00, 10, FF, CODE_ROOT_MEMORY_USAGE);	//	request memory usage
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 00, 11, FF, CODE_ROOT_MEMORY_MIRROR);	
		constexpr static obis	DEFINE_OBIS_CODE(00, 80, 80, 00, 12, FF, CODE_ROOT_MEMORY_TMP);

		constexpr static obis	DEFINE_OBIS_CODE(81, 02, 00, 07, 00, FF, CODE_ROOT_CUSTOM_INTERFACE);	//	see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle
		constexpr static obis	DEFINE_OBIS_CODE(81, 02, 00, 07, 10, FF, CODE_ROOT_CUSTOM_PARAM);	//	see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle 
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 00, 06, 10, FF, CODE_ROOT_WAN);	//	see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status 
		//constexpr static obis	DEFINE_OBIS_CODE(81, 04, 00, 06, 10, FF, CODE_ROOT_WAN_PARAM);	//	see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter 
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 02, 07, 00, FF, CODE_ROOT_GSM);	//	see: Datenstruktur zum Lesen/Setzen der GSM Parameter 
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 0D, 07, 00, FF, CODE_ROOT_GPRS_PARAM);	//	see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter 

		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 0F, 06, 00, FF, CODE_ROOT_W_MBUS_STATUS);	//	see: 7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
		constexpr static obis	DEFINE_OBIS_CODE(81, 46, 00, 00, 02, FF, CODE_ADDRESSED_PROFILE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 47, 17, 07, 00, FF, CODE_PUSH_TARGET);	//	push target name
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 00, 00, 00, 00, COMPUTER_NAME);	
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 00, 32, 02, 01, LAN_DHCP_ENABLED);	//	[bool]
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 0D, 06, 00, FF, CODE_ROOT_LAN_DSL);	//	see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter
		constexpr static obis	DEFINE_OBIS_CODE(81, 49, 0D, 06, 00, FF, CODE_ROOT_IPT_STATE);	//	see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status 
		constexpr static obis	DEFINE_OBIS_CODE(81, 49, 0D, 07, 00, FF, CODE_ROOT_IPT_PARAM);	//	see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter 

		//	ip-t status
		//constexpr static obis	DEFINE_OBIS_CODE(81, 49, 17, 07, 00, NN, CODE_ROOT_IPT_STATE_ADDRESS);	//	IP adress
		//constexpr static obis	DEFINE_OBIS_CODE(81, 49, 1A, 07, 00, NN, CODE_ROOT_IPT_STATE_PORT_LOCAL);	//	local port
		//constexpr static obis	DEFINE_OBIS_CODE(81, 49, 19, 07, 00, NN, CODE_ROOT_IPT_STATE_PORT_REMOTE);	//	remote port

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 00, 00, 13, CODE_PEER_ADDRESS_WANGSM);	//	peer address: WAN/GSM
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 00, 00, FF, CODE_PEER_ADDRESS);	//	unit is 255

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 00, 00, CODE_VERSION);		//	[string]
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 00, 02, CODE_FILE_NAME);	//	[string]
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 00, 03, CODE_MSG_COUNTER);	//	[u32] Anzahl aller Nachrichten zur Übertragung des Binary
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 00, 04, CODE_MSG_LAST);	//	[u32] Nummer der zuletzt erfolgreich übertragenen Nachricht des	Binary
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 00, 05, CODE_MSG_NUMBER);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 02, FF, CODE_BLOCK_NUMBER);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 03, FF, CODE_BINARY_DATA);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 01, 16, FF, FF, CODE_ROOT_NEW_DEVICES);	//	new active devices
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 26, FF, FF, CODE_ROOT_INVISIBLE_DEVICES);	//	not longer visible devices
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 06, FF, FF, CODE_ROOT_VISIBLE_DEVICES);	//	visible devices (Liste der sichtbaren Sensoren)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 06, 01, FF, CODE_LIST_1_VISIBLE_DEVICES);	//	1. Liste der sichtbaren Sensoren
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 11, 06, FF, FF, CODE_ROOT_ACTIVE_DEVICES);	//	active devices (Liste der aktiven Sensoren)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 11, 06, 01, FF, CODE_LIST_1_ACTIVE_DEVICES);	//	1. Liste der aktiven Sensoren)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 11, 06, FB, FF, CODE_ACTIVATE_DEVICE);	//	activate meter/sensor
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 11, 06, FC, FF, CODE_DEACTIVATE_DEVICE);	//	deactivate meter/sensor
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 11, 06, FD, FF, CODE_DELETE_DEVICE);	//	delete meter/sensor
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 12, 06, FF, FF, CODE_ROOT_DEVICE_INFO);	//	extended device information
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 81, 60, FF, FF, CODE_ROOT_ACCESS_RIGHTS);	//	see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte 

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 81, 01, FF, CODE_ROOT_FILE_TRANSFER);	//	7.3.2.28 Datenstruktur zum remote Firmware-/Datei-Download (Übertragung) 
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 81, 0E, FF, DATA_FIRMWARE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 81, 0C, FF, DATA_FILENAME);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 81, 0F, FF, DATA_FILENAME_INDIRECT);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 81, 0D, FF, DATA_APPLICATION);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 81, 10, FF, DATA_APPLICATION_INDIRECT);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 81, FF, DATA_IP_ADDRESS);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 01, FF, CODE_ROOT_DEVICE_IDENT);	//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation: firmware, file, application) 
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 02, FF, CODE_DEVICE_CLASS);	//	Geräteklasse (OBIS code or '2D 2D 2D')
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 03, FF, DATA_MANUFACTURER);	//	[string]
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 04, FF, CODE_SERVER_ID);		//	Server ID 
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 05, FF, DATA_PUBLIC_KEY);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 06, FF, CODE_ROOT_FIRMWARE);	//	Firmware
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 09, FF, CODE_HARDWARE);		//	hardware equipment (charge, type, ...)

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 08, FF, CODE_DEVICE_KERNEL);	//	[string]
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 0E, FF, CODE_DEVICE_ACTIVATED);

		//	device classes
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 41, FF, DEV_CLASS_BASIC_DIRECT);	//	3 x 230 /400 V and 5 (100) A 
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 42, FF, DEV_CLASS_BASIC_SEMI);	//	3 x 230 /400 V and 1 (6) A
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 43, FF, DEV_CLASS_BASIC_INDIRECT);	//	3 x  58 / 100 V and 1 (6) A 
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 45, FF, DEV_CLASS_IW);	//	IW module
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 46, FF, DEV_CLASS_PSTN);	//	PSTN
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 47, FF, DEV_CLASS_GPRS); //	or PLC
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 48, FF, DEV_CLASS_KM);		//	KM module (LAN/DSL)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 49, FF, DEV_CLASS_NK);		//	NK/HS
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 4A, FF, DEV_CLASS_EXTERN);		//	external load profile collector 
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 4B, FF, DEV_CLASS_RESERVED);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 4F, FF, DEV_CLASS_LAN);	//	see DEV_CLASS_MUC_LAN
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 50, FF, DEV_CLASS_eHZ);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 52, FF, DEV_CLASS_3HZ);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 53, FF, DEV_CLASS_MUC_LAN);	// (MUC - LAN / DSL)
		
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 83, 82, 01, CODE_REBOOT);	//	request reboot

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 00, FF, CODE_ROOT_SENSOR_PARAMS);	//	properties of data sensor/actor (Eigenschaften eines Datenspiegels)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 01, FF, CODE_ROOT_SENSOR_BITMASK);	//	Bitmask to define bits that will be transferred into log
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 02, FF, CODE_AVERAGE_TIME_MS);	//	average time between two received data records (milliseconds)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 20, FF, CODE_ROOT_DATA_COLLECTOR);	//	properties of data collector (Eigenschaften eines Datensammlers)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 21, FF, DATA_COLLECTOR_ACTIVE);	//	true/false
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 22, FF, DATA_COLLECTOR_SIZE);		//	max. table size
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 04, FF, CODE_TIME_REFERENCE);	//	[u8] 0 == UTC, 1 == UTC + time zone, 2 == local time
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 87, 81, FF, DATA_COLLECTOR_PERIOD);	//	 (u32) register period in seconds (0 == event driven)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 01, FF, CODE_ROOT_NTP);	//	NTP configuration
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 02, FF, CODE_NTP_SERVER);	//	List of NTP servers
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 03, FF, CODE_NTP_PORT);	//	[u16] NTP port (123)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 04, FF, CODE_NTP_TZ);		//	[u32] timezone
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 05, FF, CODE_NTP_OFFSET);	//	[sec] Offset to transmission of the signal for synchronization
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 06, FF, CODE_NTP_ACTIVE);	//	[bool] NTP enabled/disables
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 10, FF, CODE_ROOT_DEVICE_TIME);	//	device time

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 00, FF, CODE_IF_1107);	 //	1107 interface (IEC 62056-21)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 01, FF, CODE_IF_1107_ACTIVE); //	(bool) - if true 1107 interface active otherwise SML interface active
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 02, FF, CODE_IF_1107_LOOP_TIME); //	(u) - Loop timeout in seconds
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 03, FF, CODE_IF_1107_RETRIES); //	(u) - Retry count
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 04, FF, CODE_IF_1107_MIN_TIMEOUT); //	(u) - Minimal answer timeout(300)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 05, FF, CODE_IF_1107_MAX_TIMEOUT); //	(u) - Maximal answer timeout(5000)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 06, FF, CODE_IF_1107_MAX_DATA_RATE); //	(u) - Maximum data bytes(10240)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 07, FF, CODE_IF_1107_RS485); //	(bool) - if true RS 485, otherwise RS 323
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 08, FF, CODE_IF_1107_PROTOCOL_MODE); //	(u) - Protocol mode(A ... D)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 09, FF, CODE_IF_1107_METER_LIST); // Liste der abzufragenden 1107 Zähler
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 10, FF, CODE_IF_1107_AUTO_ACTIVATION); //(True)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 11, FF, CODE_IF_1107_TIME_GRID); //	time grid of load profile readout in seconds
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 13, FF, CODE_IF_1107_TIME_SYNC); //	time sync in seconds
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 14, FF, CODE_IF_1107_MAX_VARIATION); //(seconds)

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 0A, FF, CODE_IF_1107_METER_ID); //	(string)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 0B, FF, CODE_IF_1107_BAUDRATE); //	(u)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 0C, FF, CODE_IF_1107_ADDRESS); //	(string)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 0D, FF, CODE_IF_1107_P1); //	(string)
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 93, 0E, FF, CODE_IF_1107_W5); //	(string)

																								  //
		//	Interfaces
		//
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 17, 07, 00, FF, CODE_IF_LAN_DSL);	// see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 02, 07, 00, FF, CODE_IF_GSM);
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 0D, 07, 00, FF, CODE_IF_GPRS);
		constexpr static obis	DEFINE_OBIS_CODE(81, 02, 00, 07, 00, FF, CODE_IF_USER);	//	Endkundenschnittstelle:
		//constexpr static obis	DEFINE_OBIS_CODE(81, 49, 0D, 07, 00, FF, CODE_IF_IPT);	//	 same as OBIS_CODE_ROOT_IPT_PARAM
		constexpr static obis	DEFINE_OBIS_CODE(81, 05, 0D, 07, 00, FF, CODE_IF_EDL);		//	M-Bus EDL (RJ10)
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 00, FF, CODE_IF_wMBUS);	//	Wireless M-BUS:
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 18, 07, 00, FF, CODE_IF_PLC);
		//constexpr static obis	DEFINE_OBIS_CODE(81, 05, 0D, 07, 00, FF, CODE_IF_SyM2);	//	same as CODE_IF_EDL
		constexpr static obis	DEFINE_OBIS_CODE(81, 00, 00, 09, 0B, 00, ACT_SENSOR_TIME);	//	actSensorTime - time delivered from sensor 

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 89, E1, FF, CLASS_OP_LOG);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, 89, E2, FF, CLASS_EVENT);	//	Ereignis (uint32)

		//
		//	wMBus - 81 06 0F 06 00 FF
		//	7.3.1.23 Datenstruktur zum Lesen des W-MBUS-Status 
		//
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 00, 00, 01, 00, W_MBUS_ADAPTER_MANUFACTURER);	//	[string]
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 00, 00, 03, 00, W_MBUS_ADAPTER_ID);
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 00, 02, 00, 00, W_MBUS_FIRMWARE);	//	[string]
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 00, 02, 03, FF, W_MBUS_HARDWARE);	//	[string]

		//
		//	Wireless M-BUS config
		//	81 06 19 07 00 FF, CODE_IF_wMBUS
		//
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 01, FF, W_MBUS_PROTOCOL);
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 02, FF, W_MBUS_MODE_S);	//	u8
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 03, FF, W_MBUS_MODE_T);	//	u8
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 04, FF, W_MBUS_MODE_C);	//	alternating
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 05, FF, W_MBUS_MODE_N);	//	parallel
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 06, FF, W_MBUS_MODE_S_SYN);	//	Used for synchronized messages in meters

		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 27, 32, 03, 01, W_MBUS_REBOOT);	//	u32
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 04, FF, W_MBUS_POWER);
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 19, 07, 11, FF, W_MBUS_INSTALL_MODE);

		//	Spannung - voltage
		//	Strom - current
		//	Leistungsfaktor - power factor
		//	Scheinleistung - apparent power
		//	Wirkleistung - effective (or active) power

		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 01, 08, 00, FF, REG_POS_AE_NO_TARIFF);	//	1.8.0 - Zählwerk pos. Wirkenergie, 	tariflos
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 01, 08, 01, FF, REG_POS_AE_T1);		//	1.8.1 - Zählwerk pos. Wirkenergie, Tarif 1
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 01, 08, 02, FF, REG_POS_AE_T2);		//	1.8.2 - Zählwerk pos. Wirkenergie, Tarif 2
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 02, 08, 00, FF, REG_NEG_AE_NO_TARIFF);	//	2.8.0 - Zählwerk neg. Wirkenergie, 	tariflos
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 02, 08, 01, FF, REG_NEG_AE_T1);		//	2.8.1 - Zählwerk neg. Wirkenergie, Tarif 1
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 02, 08, 02, FF, REG_NEG_AE_T2);		//	2.8.2 - Zählwerk neg. Wirkenergie, Tarif 2
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 0F, 07, 00, FF, REG_CUR_POS_AE);		//	15.7.0 - Aktuelle positive Wirkleistung, Betrag
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 10, 07, 00, ff, REG_CUR_AP);		//	aktuelle Wirkleistung 

		//	01 00 24 07 00 FF: Wirkleistung L1
		//01 00 38 07 00 FF: Wirkleistung L2
		//01 00 4C 07 00 FF: Wirkleistung L3
		//01 00 60 32 00 02: Aktuelle Chiptemperatur
		//01 00 60 32 00 03: Minimale Chiptemperatur
		//01 00 60 32 00 04: Maximale Chiptemperatur
		//01 00 60 32 00 05: Gemittelte Chiptemperatur
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 1F, 07, 00, FF, REG_CURRENT_L1);	//	Strom L1
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 20, 07, 00, FF, REG_VOLTAGE_L1);	//	Spannung L1
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 33, 07, 00, FF, REG_CURRENT_L2);	//	Strom L2
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 34, 07, 00, FF, REG_VOLTAGE_L2);	//	Spannung L2
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 47, 07, 00, FF, REG_CURRENT_L3);	//	Strom L3
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 48, 07, 00, FF, REG_VOLTAGE_L3);	//	Spannung L3 
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 60, 32, 03, 03, REG_VOLTAGE_MIN);	//	Spannungsminimum
		constexpr static obis	DEFINE_OBIS_CODE(01, 00, 60, 32, 03, 04, REG_VOLTAGE_MAX);	//	Spannungsmaximum


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
		constexpr static obis	DEFINE_OBIS_CODE(81, 00, 60, 05, 00, 00, CLASS_OP_LOG_STATUS_WORD);	
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 2B, 07, 00, 00, CLASS_OP_LOG_FIELD_STRENGTH);
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 1A, 07, 00, 00, CLASS_OP_LOG_CELL);	//	aktuelle Zelleninformation (uint16)
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 17, 07, 00, 00, CLASS_OP_LOG_AREA_CODE);	//	aktueller Location - oder Areacode(uint16)
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 0D, 06, 00, 00, CLASS_OP_LOG_PROVIDER);	//	aktueller Provider-Identifier(uint32)

		//
		//	attention codes
		//
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 00, ATTENTION_UNKNOWN_ERROR);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 01, ATTENTION_UNKNOWN_SML_ID);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 02, ATTENTION_NOT_AUTHORIZED);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 03, ATTENTION_NO_SERVER_ID);	//	unable to find recipient for request
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 04, ATTENTION_NO_REQ_FIELD);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 05, ATTENTION_CANNOT_WRITE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 06, ATTENTION_CANNOT_READ);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 07, ATTENTION_COMM_ERROR);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 08, ATTENTION_PARSER_ERROR);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 09, ATTENTION_OUT_OF_RANGE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 0A, ATTENTION_NOT_EXECUTED);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 0B, ATTENTION_INVALID_CRC);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 0C, ATTENTION_NO_BROADCAST);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 0D, ATTENTION_UNEXPECTED_MSG);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 0E, ATTENTION_UNKNOWN_OBIS_CODE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 0F, ATTENTION_UNSUPPORTED_DATA_TYPE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 10, ATTENTION_ELEMENT_NOT_OPTIONAL);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 11, ATTENTION_NO_ENTRIES);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 12, ATTENTION_END_LIMIT_BEFORE_START);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 13, ATTENTION_NO_ENTRIES_IN_RANGE);	//	range is empty - not the profile
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FE, 14, ATTENTION_MISSING_CLOSE_MSG);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FD, 00, ATTENTION_OK);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, C7, C7, FD, 01, ATTENTION_JOB_IS_RUNNINNG);


		//
		//	list types
		//
		constexpr static obis	DEFINE_OBIS_CODE(99, 00, 00, 00, 00, 03, LIST_CURRENT_DATA_RECORD);
		constexpr static obis	DEFINE_OBIS_CODE(99, 00, 00, 00, 00, 04, LIST_SERVICES);

		//
		//	source for log entries
		//
		constexpr static obis	DEFINE_OBIS_CODE(81, 01, 00, 00, 00, FF, LOG_SOURCE_ETH_AUX);
		constexpr static obis	DEFINE_OBIS_CODE(81, 02, 00, 00, 00, FF, LOG_SOURCE_ETH_CUSTOM);
		constexpr static obis	DEFINE_OBIS_CODE(81, 03, 00, 00, 00, FF, LOG_SOURCE_RS232);
		constexpr static obis	DEFINE_OBIS_CODE(81, 04, 00, 00, 00, FF, LOG_SOURCE_ETH);	//	WAN interface
		constexpr static obis	DEFINE_OBIS_CODE(81, 05, 00, 00, 00, FF, LOG_SOURCE_eHZ);	
		constexpr static obis	DEFINE_OBIS_CODE(81, 06, 00, 00, 00, FF, LOG_SOURCE_wMBUS);

		constexpr static obis	DEFINE_OBIS_CODE(81, 41, 00, 00, 00, FF, LOG_SOURCE_IP);

		constexpr static obis	DEFINE_OBIS_CODE(81, 42, 00, 00, 00, 01, LOG_SOURCE_SML_EXT);
		constexpr static obis	DEFINE_OBIS_CODE(81, 42, 00, 00, 00, 02, LOG_SOURCE_SML_CUSTOM);
		constexpr static obis	DEFINE_OBIS_CODE(81, 42, 00, 00, 00, 03, LOG_SOURCE_SML_SERVICE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 42, 00, 00, 00, 04, LOG_SOURCE_SML_WAN);
		constexpr static obis	DEFINE_OBIS_CODE(81, 42, 00, 00, 00, 05, LOG_SOURCE_SML_eHZ);
		constexpr static obis	DEFINE_OBIS_CODE(81, 42, 00, 00, 00, 06, LOG_SOURCE_SML_wMBUS);

		constexpr static obis	DEFINE_OBIS_CODE(81, 45, 00, 00, 00, FF, LOG_SOURCE_PUSH_SML);
		constexpr static obis	DEFINE_OBIS_CODE(81, 46, 00, 00, 00, FF, LOG_SOURCE_PUSH_IPT_SOURCE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 47, 00, 00, 00, FF, LOG_SOURCE_PUSH_IPT_SINK);
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 00, 00, 00, 01, LOG_SOURCE_WAN_DHCP);
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 00, 00, 00, 02, LOG_SOURCE_WAN_IP);
		constexpr static obis	DEFINE_OBIS_CODE(81, 48, 00, 00, 00, 03, LOG_SOURCE_WAN_PPPoE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 49, 00, 00, 00, 01, LOG_SOURCE_WAN_IPT_CONTROLLER);
		constexpr static obis	DEFINE_OBIS_CODE(81, 49, 00, 00, 00, 02, LOG_SOURCE_WAN_IPT);
		constexpr static obis	DEFINE_OBIS_CODE(81, 4A, 00, 00, 00, FF, LOG_SOURCE_WAN_NTP);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 02, 00, 01, SET_START_FW_UPDATE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 00, 03, 00, 01, SET_DISPATCH_FW_UPDATE);
		

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 00, 00, 01, LOG_SOURCE_LOG);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 00, 00, 02, LOG_SOURCE_SCM);	//	internal software function
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 00, 00, 03, LOG_SOURCE_UPDATE);
		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 00, 00, 04, LOG_SOURCE_SMLC);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 00, 00, 0C, LOG_SOURCE_LEDIO);

		constexpr static obis	DEFINE_OBIS_CODE(81, 81, 10, 00, 00, 14, LOG_SOURCE_WANPLC);


		/**
		 * @return a short description of the OBIS code if available
		 */
		const char* get_name(obis const&);

		/**
		 * @return a short description of the OBIS code if available
		 */
		const char* get_attention_name(obis const&);

		/**
		 * @return name of teh LSM event
		 */
		const char* get_LSM_event_name(std::uint32_t);

	}	//	sml
}	//	node


#endif	
