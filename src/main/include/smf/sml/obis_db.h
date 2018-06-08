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
	node::sml::obis (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

namespace node
{
	namespace sml
	{

		//
		//	Lastgänge (profiles)
		//	The OBIS code to encode profiles is 81 81 C7 8A 83 FF
		//

		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 03, FF, DATA_MANUFACTURER);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 05, FF, DATA_PUBLIC_KEY);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 03, FF, DATA_AES_KEY);
		const static obis	DEFINE_OBIS_CODE(81, 81, 61, 3C, 01, FF, DATA_USER_NAME);
		const static obis	DEFINE_OBIS_CODE(81, 81, 61, 3C, 02, FF, DATA_USER_PWD);

		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 10, FF, PROFILE_1_MINUTE);	//	1 minute
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 11, FF, PROFILE_15_MINUTE);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 12, FF, PROFILE_60_MINUTE);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 13, FF, PROFILE_24_HOUR);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 14, FF, PROFILE_LAST_2_HOURS);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 15, FF, PROFILE_LAST_WEEK);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 16, FF, PROFILE_1_MONTH);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 17, FF, PROFILE_1_YEAR);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 18, FF, PROFILE_INITIAL);

		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 8A, 83, FF, PROFILE);	//	encode profiles

		//
		/**
		 * @return true id the 4th first bytes are 0x81, 0x81, 0xC7, 0x86
		 */
		bool is_profile(obis const&);

		//
		//	root elements
		//
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 00, 10, FF, CODE_ROOT_MEMORY_USAGE);	//	request memory usage

		const static obis	DEFINE_OBIS_CODE(81, 02, 00, 07, 00, FF, CODE_ROOT_CUSTOM_INTERFACE);	//	see: 7.3.1.3 Datenstruktur zum Lesen / Setzen der Parameter für die Kundenschnittstelle
		const static obis	DEFINE_OBIS_CODE(81, 02, 00, 07, 10, FF, CODE_ROOT_CUSTOM_PARAM);	//	see: 7.3.1.4 Datenstruktur für dynamischen Eigenschaften der Endkundenschnittstelle 
		const static obis	DEFINE_OBIS_CODE(81, 04, 00, 06, 10, FF, CODE_ROOT_WAN);	//	see: 7.3.1.5 Datenstruktur zur Abfrage des WAN Status 
		//const static obis	DEFINE_OBIS_CODE(81, 04, 00, 06, 10, FF, CODE_ROOT_WAN_PARAM);	//	see: 7.3.1.6 Datenstruktur zum Lesen/Setzen der WAN Parameter 
		const static obis	DEFINE_OBIS_CODE(81, 04, 02, 07, 00, FF, CODE_ROOT_GSM);	//	see: Datenstruktur zum Lesen/Setzen der GSM Parameter 
		const static obis	DEFINE_OBIS_CODE(81, 04, 0D, 07, 00, FF, CODE_ROOT_GPRS_PARAM);	//	see: Datenstruktur zum Lesen / Setzen der Provider-abhängigen GPRS-Parameter 
		const static obis	DEFINE_OBIS_CODE(81, 48, 0D, 06, 00, FF, CODE_ROOT_LAN_DSL);	//	see: 7.3.1.19 Datenstruktur zur Abfrage dynamischer LAN/DSL- Betriebsparameter
		const static obis	DEFINE_OBIS_CODE(81, 49, 0D, 06, 00, FF, CODE_ROOT_IPT_STATE);	//	see: 7.3.1.8 Datenstruktur zur Abfrage des IPT Status 
		const static obis	DEFINE_OBIS_CODE(81, 49, 0D, 07, 00, FF, CODE_ROOT_IPT_PARAM);	//	see: 7.3.1.9 Datenstruktur zur Lesen/Setzen der IPT Parameter 
		const static obis	DEFINE_OBIS_CODE(81, 81, 01, 16, FF, FF, CODE_ROOT_NEW_DEVICES);	//	new active devices
		const static obis	DEFINE_OBIS_CODE(81, 81, 10, 26, FF, FF, CODE_ROOT_INVISIBLE_DEVICES);	//	not longer visible devices
		const static obis	DEFINE_OBIS_CODE(81, 81, 10, 06, FF, FF, CODE_ROOT_VISIBLE_DEVICES);	//	visible devices (Liste der sichtbaren Sensoren)
		const static obis	DEFINE_OBIS_CODE(81, 81, 11, 06, FF, FF, CODE_ROOT_ACTIVE_DEVICES);	//	active devices (Liste der aktiven Sensoren)
		const static obis	DEFINE_OBIS_CODE(81, 81, 12, 06, FF, FF, CODE_ROOT_DEVICE_INFO);	//	extended device information
		const static obis	DEFINE_OBIS_CODE(81, 81, 81, 60, FF, FF, CODE_ROOT_ACCESS_RIGHTS);	//	see: 7.3.1.2 Datenstruktur zur Parametrierung der Rollen / Benutzerrechte 
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 01, FF, CODE_ROOT_DEVICE_IDENT);	//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation) 
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 02, FF, CODE_DEVICE_CLASS);	//	Geräteklasse 
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 04, FF, CODE_SERVER_ID);	//	Server ID der sichtbaren Komponenten
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 00, FF, CODE_ROOT_SENSOR_PROPERTY);	//	properties of data sensor/actor (Eigenschaften eines Datenspiegels)
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 01, FF, CODE_ROOT_NTP);
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 88, 10, FF, CODE_ROOT_DEVICE_TIME);	//	device time
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 20, FF, CODE_ROOT_DATA_COLLECTOR);	//	properties of data collector (Eigenschaften eines Datensammlers)
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 86, 02, FF, CODE_AVERAGE_TIME_MS);	//	average time between two received data records (milliseconds)
		const static obis	DEFINE_OBIS_CODE(81, 81, C7, 83, 82, 01, CODE_REBOOT);	//	request reboot

		//
		//	Interfaces
		//
		const static obis	DEFINE_OBIS_CODE(81, 48, 17, 07, 00, FF, CODE_IF_LAN_DSL);	// see: 7.3.1.18 Datenstruktur zum Lesen / Setzen der LAN/DSL-Parameter
		const static obis	OBIS_CODE_IF_GSM(0x81, 0x04, 0x02, 0x07, 0x00, 0xFF);
		const static obis	OBIS_CODE_IF_GPRS(0x81, 0x04, 0x0D, 0x07, 0x00, 0xFF);
		const static obis	OBIS_CODE_IF_USER(0x81, 0x02, 0x00, 0x07, 0x00, 0xFF);	//	Endkundenschnittstelle:
		const static obis	OBIS_CODE_IF_IPT(0x81, 0x49, 0x0D, 0x07, 0x00, 0xFF);
		const static obis	OBIS_CODE_IF_EDL(0x81, 0x05, 0x0D, 0x07, 0x00, 0xFF);		//	M-Bus EDL
		const static obis	OBIS_CODE_IF_wMBUS(0x81, 0x06, 0x19, 0x07, 0x00, 0xFF);	//	Wireless M-BUS:
		const static obis	OBIS_CODE_IF_PLC(0x81, 0x04, 0x18, 0x07, 0x00, 0xFF);
		const static obis	OBIS_CODE_IF_SyM2(0x81, 0x05, 0x0D, 0x07, 0x00, 0xFF);	//	Erweiterungsschnittstelle:
		const static obis	DEFINE_OBIS_CODE(81, 00, 00, 09, 0B, 00, ACT_SENSOR_TIME);	//	actSensorTime - time delivered from sensor 

		const static obis	OBIS_CLASS_OP_LOG(0x81, 0x81, 0xC7, 0x89, 0xE1, 0xFF);
		const static obis	OBIS_CLASS_EVENT(0x81, 0x81, 0xC7, 0x89, 0xE2, 0xFF);	//	Ereignis (uint32)
		const static obis	OBIS_CLASS_STATUS(0x00, 0x80, 0x80, 0x11, 0xA0, 0xFF);	//	see: 2.2.1.3 Status der Aktoren (Kontakte)

		const static obis	DEFINE_OBIS_CODE(00, 00, 60, 01, 00, FF, SERIAL_NR);	//	Serial number I (assigned by the manufacturer).
		const static obis	DEFINE_OBIS_CODE(00, 00, 60, 01, 01, FF, SERIAL_NR_SECOND);	//	Serial number II (assigned by the manufacturer).
		const static obis	DEFINE_OBIS_CODE(00, 00, 61, 61, 00, FF, MBUS_STATE);   //	Status according to EN13757-3

		//	Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
		const static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 00, FF, SERVER_ID_1_1);
		const static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 01, FF, SERVER_ID_1_2);		//	Identifikationsnummer 1.2
		const static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 02, FF, SERVER_ID_1_3);		//	Identifikationsnummer 1.3
		const static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 03, FF, SERVER_ID_1_4);		//	Identifikationsnummer 1.4
		const static obis	DEFINE_OBIS_CODE(01, 00, 00, 00, 09, FF, DEVICE_ID);	//	encode profiles

		const static obis	DEFINE_OBIS_CODE(01, 00, 00, 09, 0B, 00, CURRENT_UTC);	//	readout time in UTC
		//const static obis	OBIS_CLASS_TIMESTAMP_UTC(0x01, 0x00, 0x00, 0x09, 0x0B, 0x00);	//	timestamp UTC - SML_Time (Zeitsynchronisation per NTPZugriff erfolgt)


		//	Spannung - voltage
		//	Strom - current
		//	Leistungsfaktor - power factor
		//	Scheinleistung - apparent power
		//	Wirkleistung - effective (or active) power

		const static obis	DEFINE_OBIS_CODE(01, 00, 01, 08, 00, FF, REG_POS_AE_NO_TARIFF);	//	Zählwerk pos. Wirkenergie, 	tariflos
		const static obis	DEFINE_OBIS_CODE(01, 00, 01, 08, 01, FF, REG_POS_AE_T1);	//	Zählwerk pos. Wirkenergie, Tarif 1
		const static obis	DEFINE_OBIS_CODE(01, 00, 01, 08, 02, FF, REG_POS_AE_T2);	//	Zählwerk pos. Wirkenergie, Tarif 2
		const static obis	DEFINE_OBIS_CODE(01, 00, 02, 08, 00, FF, REG_NEG_AE_NO_TARIFF);	//	Zählwerk neg. Wirkenergie, 	tariflos
		const static obis	DEFINE_OBIS_CODE(01, 00, 02, 08, 01, FF, REG_NEG_AE_T1);	//	Zählwerk neg. Wirkenergie, Tarif 1
		const static obis	DEFINE_OBIS_CODE(01, 00, 02, 08, 02, FF, REG_NEG_AE_T2); //	Zählwerk neg. Wirkenergie, Tarif 2		//const static obis	DEFINE_OBIS_CODE(01, 00, 0F, 07, 00, FF, REG_CUR_POS_AE);	//	Aktuelle pos.	Wirkleistung 	Betrag
		const static obis	DEFINE_OBIS_CODE(01, 00, 0F, 07, 00, FF, REG_CUR_POS_AE);
		const static obis	DEFINE_OBIS_CODE(01, 00, 10, 07, 00, ff, REG_CUR_AP);	//	aktuelle Wirkleistung 

		//	01 00 24 07 00 FF: Wirkleistung L1
		//01 00 38 07 00 FF: Wirkleistung L2
		//01 00 4C 07 00 FF: Wirkleistung L3
		//01 00 60 32 00 02: Aktuelle Chiptemperatur
		//01 00 60 32 00 03: Minimale Chiptemperatur
		//01 00 60 32 00 04: Maximale Chiptemperatur
		//01 00 60 32 00 05: Gemittelte Chiptemperatur
		const static obis	DEFINE_OBIS_CODE(01, 00, 1F, 07, 00, FF, REG_CURRENT_L1);	//	Strom L1
		const static obis	DEFINE_OBIS_CODE(01, 00, 20, 07, 00, FF, REG_VOLTAGE_L1);	//	Spannung L1
		const static obis	DEFINE_OBIS_CODE(01, 00, 33, 07, 00, FF, REG_CURRENT_L2);	//	Strom L2
		const static obis	DEFINE_OBIS_CODE(01, 00, 34, 07, 00, FF, REG_VOLTAGE_L2);	//	Spannung L2
		const static obis	DEFINE_OBIS_CODE(01, 00, 47, 07, 00, FF, REG_CURRENT_L3);	//	Strom L3
		const static obis	DEFINE_OBIS_CODE(01, 00, 48, 07, 00, FF, REG_VOLTAGE_L3);	//	Spannung L3 
		const static obis	DEFINE_OBIS_CODE(01, 00, 60, 32, 03, 03, REG_VOLTAGE_MIN);	//	Spannungsminimum
		const static obis	DEFINE_OBIS_CODE(01, 00, 60, 32, 03, 04, REG_VOLTAGE_MAX);	//	Spannungsmaximum


		//		const static obis	OBIS_CLASS_OP_LOG_EVENT(0x81, 0x81, 0xC7, 0x89, 0xE2, 0xFF);	//	Ereignis (uint32)
		const static obis	OBIS_CLASS_OP_LOG_PEER_ADDRESS(0x81, 0x81, 0x00, 0x00, 0x00, 0xFF);	//	Peer Adresse (octet string)
		const static obis	OBIS_CLASS_OP_MONITORING_STATUS(0x00, 0x80, 0x80, 0x11, 0x13, 0x01);	//	Überwachungsstatus (uint8) - tatsächlich i32

		/**
		 *	Statuswort (per Schreiben ist das Rücksetzen ausgewählter Statusbits) Unsigned64
		 *	see also http://www.sagemcom.com/fileadmin/user_upload/Energy/Dr.Neuhaus/Support/ZDUE-MUC/Doc_communs/ZDUE-MUC_Anwenderhandbuch_V2_5.pdf
		 *	bit meaning
		 *	0	always 0
		 *	1	always 1
		 *	2-7	always 0
		 *	8	1 if error was detected
		 *	9	1 if restart was triggered by watchdog reset
		 *	10	0 if IP address is available (DHCP)
		 *	11	0 if ethernet link is available
		 *	12	always 0
		 *	13	0 if authorized on IP-T server
		 *	14	1 ic case of out of memory
		 *	15	always 0
		 *	16	1 if Service interface is available (Kundenschnittstelle)
		 *	17	1 if extension interface is available (Erweiterungs-Schnittstelle)
		 *	18	1 if Wireless M-Bus interface is available
		 *	19	1 if PLC is available
		 *	20-31	always 0
		 *	32	1 if time base is unsure
		 */
		const static obis	DEFINE_OBIS_CODE(81, 00, 60, 05, 00, 00, CLASS_OP_LOG_STATUS_WORD);	
		const static obis	DEFINE_OBIS_CODE(81, 04, 2B, 07, 00, 00, CLASS_OP_LOG_FIELD_STRENGTH);
		const static obis	OBIS_CLASS_OP_LOG_CELL(0x81, 0x04, 0x1A, 0x07, 0x00, 0x00);	//	aktuelle Zelleninformation (uint16)
		const static obis	OBIS_CLASS_OP_LOG_AREA_CODE(0x81, 0x04, 0x17, 0x07, 0x00, 0x00);	//	aktueller Location - oder Areacode(uint16)
		const static obis	OBIS_CLASS_OP_LOG_PROVIDER(0x81, 0x04, 0x0D, 0x06, 0x00, 0x00);	//	aktueller Provider-Identifier(uint32)

		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 10, 00, 01, CLASS_OP_LSM_STATUS);	//	LSM status
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 10, FF, CLASS_OP_LSM_ACTOR_ID);	//	ServerID des Aktors (uint16)
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 11, FF, CLASS_OP_LSM_CONNECT);	//	Verbindungsstatus
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 11, 01, CLASS_OP_LSM_SWITCH);	//	Schaltanforderung
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 13, FF, CLASS_OP_LSM_FEEDBACK);	//	feedback configuration
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 14, FF, CLASS_OP_LSM_LOAD);	//	number of loads
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, 15, FF, CLASS_OP_LSM_POWER);	//	total power
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A1, FF, CLASS_OP_LSM_VERSION);	//	LSM version
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A2, FF, CLASS_OP_LSM_TYPE);	//	LSM type
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A9, 01, CLASS_OP_LSM_ACTIVE_RULESET); //	active ruleset
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A9, 01, CLASS_RADIO_KEY);	//	or radio key
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 11, A9, 02, CLASS_OP_LSM_PASSIVE_RULESET);
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 14, 03, FF, CLASS_OP_LSM_JOB);//	Schaltauftrags ID ((octet string)	
		const static obis	DEFINE_OBIS_CODE(00, 80, 80, 14, 20, FF, CLASS_OP_LSM_POSITION);	//	current position


		//
		//	attention codes
		//
		const static obis	OBIS_ATTENTION_UNKNOWN_ERROR(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x00);
		const static obis	OBIS_ATTENTION_UNKNOWN_SML_ID(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x01);
		const static obis	OBIS_ATTENTION_NOT_AUTHORIZED(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x02);
		const static obis	OBIS_ATTENTION_NO_SERVER_ID(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x03);
		const static obis	OBIS_ATTENTION_NO_REQ_FIELD(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x04);
		const static obis	OBIS_ATTENTION_CANNOT_WRITE(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x05);
		const static obis	OBIS_ATTENTION_CANNOT_READ(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x06);
		const static obis	OBIS_ATTENTION_COMM_ERROR(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x07);
		const static obis	OBIS_ATTENTION_PARSER_ERROR(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x08);
		const static obis	OBIS_ATTENTION_OUT_OF_RANGE(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x09);
		const static obis	OBIS_ATTENTION_NOT_EXECUTED(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x0A);
		const static obis	OBIS_ATTENTION_INVALID_CRC(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x0B);
		const static obis	OBIS_ATTENTION_NO_BROADCAST(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x0C);
		const static obis	OBIS_ATTENTION_UNEXPECTED_MSG(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x0D);
		const static obis	OBIS_ATTENTION_UNKNOWN_OBIS_CODE(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x0E);
		const static obis	OBIS_ATTENTION_UNSUPPORTED_DATA_TYPE(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x0F);
		const static obis	OBIS_ATTENTION_ELEMENT_NOT_OPTIONAL(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x10);
		const static obis	OBIS_ATTENTION_NO_ENTRIES(0x81, 0x81, 0xC7, 0xC7, 0xFE, 0x11);
		const static obis	OBIS_ATTENTION_OK(0x81, 0x81, 0xC7, 0xC7, 0xFD, 0x00);
		const static obis	OBIS_ATTENTION_JOB_IS_RUNNINNG(0x81, 0x81, 0xC7, 0xC7, 0xFD, 0x01);

		/**
		 * @return a short description of the OBIS code if available
		 */
		const char* get_name(obis const&);

		/**
		 * @return name of teh LSM event
		 */
		const char* get_LSM_event_name(std::uint32_t);

	}	//	sml
}	//	node


#endif	
