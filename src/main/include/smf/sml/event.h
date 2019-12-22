/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EVENT_H
#define NODE_SML_EVENT_H

/** @file event.h
 * 
 * OBIS code "81 81 C7 89 E2 FF" (OBIS_CLASS_EVENT) provides an u32 value that 
 * encodes an event type with informations about event type (enum event_type)
 * level and source.
 * @see 21.47 Datenstrukturen des Betriebslogbuchs
 */

#include <cstdint>

namespace node
{
	namespace sml
	{
		std::uint32_t evt_timer();

		/**
		 * voltage recovery
		 */
		std::uint32_t evt_voltage_recovery();

		/**
		 * If possible lof this event in case of power outage.
		 */
		std::uint32_t evt_power_outage();

		/** @brief 0x00100001
		 *
		 * First entry after power recovery
		 */
		std::uint32_t evt_power_recovery();

		/** @brief 0x00100003
		 *
		 * Firmware successful activated
		 */
		std::uint32_t evt_firmware_activated();

		/** @brief 0x00100004
		 *
		 * Firmware not working
		 */
		std::uint32_t evt_firmware_failure();

		/** @brief 0x00100005
		 *
		 * Firmware hash error
		 */
		std::uint32_t evt_firmware_hash_error();

		/** @brief 0x00100006
		 *
		 * Firmware type error (wrong hardware)
		 */
		std::uint32_t evt_firmware_type_error();

		/** @brief 0x00100007
		 *
		 * Firmware upload OK
		 */
		std::uint32_t evt_firmware_upload_complete();

		/** @brief 0x00800000
		 *
		 * Timer - cyclic logbook entry
		 */
		std::uint32_t evt_timer();

		/** @brief 0x00800004
		 *
		 * cyclic reset of a communication module
		 */
		std::uint32_t evt_cyclic_reset();

		/** @brief 0x00800010
		 *
		 * set system time by user
		 */
		std::uint32_t evt_system_time_set();

		std::uint32_t evt_watchdog();
		std::uint32_t evt_push_succes();
		std::uint32_t evt_push_failed();
		std::uint32_t evt_ipt_connect();
		std::uint32_t evt_ipt_disconnect();
		std::uint32_t evt_npt_connect();
		std::uint32_t evt_npt_disconnect();

		/**
		 * @return event type
		 */
		std::uint32_t evt_get_type(std::uint32_t);

		/**
		 * @return event level
		 */
		std::uint32_t evt_get_level(std::uint32_t);

		/**
		 * @return event source
		 */
		std::uint32_t evt_get_source(std::uint32_t);

		/**
		 * Types of log events
		 */
		enum event_type : std::uint32_t
		{
			LOG_CODE_PW = 0x00010001,	//	Phasenwechsel - 81 81 C7 89 E2 FF
			LOG_CODE_SA = 0x0001000F,	//	system clock "async"
			LOG_CODE_01 = 0x00100001,	//	Spannungsausfall, Das Ereignis wird nach Wiederkehr der	Versorgungsspannung als erster Eintrag in das Betriebslogbuch eingetragen
			LOG_CODE_02 = 0x00100002,	//	Spannungsausfall, Letzter Reset aufgrund eines Spannungsausfalls
			LOG_CODE_03 = 0x00100003,	//	Firmware - Aktivierung
			LOG_CODE_04 = 0x00100004,	//	Fehlerhafte Firmware
			LOG_CODE_05 = 0x00100005,	//	Firmware - Hash - Error
			LOG_CODE_06 = 0x00100006,	//	Firmware - Type - Error
			LOG_CODE_07 = 0x00100007,	//	Firmware - Upload OK
			LOG_CODE_08 = 0x00100008,	//	Ethernet PHY gestört
			LOG_CODE_MD = 0x00010011,	//	manipulation detected
			LOG_CODE_09 = 0x00100023,	//	Spannungswiederkehr
			LOG_CODE_10 = 0x00800000,	//	Timer, Zyklischer Logbucheintrag (8388608dec)
			LOG_CODE_11 = 0x00800004,	//	Periodischer Reset
			LOG_CODE_12 = 0x00800005,	//	Watchdog, Watchdog aufgetreten
			LOG_CODE_13 = 0x00800006,	//	Sync - Token erzeugt (reserved)
			LOG_CODE_14 = 0x00800007,	//	Sync - Token durchgeleitet (reserved)
			LOG_CODE_15 = 0x00800008,	//	Push - Vorgang erfolgreich
			LOG_CODE_16 = 0x00800009,	//	Push - Vorgang nicht erfolgreich
			LOG_CODE_17 = 0x0080000D,	//	Datensammler Parameter synchronisiert
			LOG_CODE_18 = 0x0080000E,	//	Push Parameter synchronisiert
			LOG_CODE_19 = 0x0080000F,	//	Benutzerrechte synchronisiert
			LOG_CODE_20 = 0x00800010,	//	Systemzeit durch Benutzer gesetzt
			LOG_CODE_21 = 0x00800011,	//	Fehler bei Aktivierung 
			LOG_CODE_22 = 0x01100008,	//	Ethernet - Link an ErweiterungsSchnittstelle aktiviert (reserved)
			LOG_CODE_23 = 0x01100009,	//	Ethernet - Link an ErweiterungsSchnittstelle deaktiviert (reserved)
			LOG_CODE_24 = 0x02100008,	//	Ethernet - Link an KundenSchnittstelle aktiviert
			LOG_CODE_25 = 0x02100009,	//	Ethernet - Link an KundenSchnittstelle deaktiviert
			LOG_CODE_26 = 0x04100001,	//	WAN - Modul eingeschaltet
			LOG_CODE_27 = 0x04100002,	//	WAN - Modul ausgeschaltet
			LOG_CODE_28 = 0x04100008,	//	WAN verfügbar, Weitverkehrsnetz erkannt
			LOG_CODE_29 = 0x04100009,	//	WAN nicht verfügbar, Weitverkehrsnetz nicht erkannt
			LOG_CODE_30 = 0x0410000A,	//	Im WAN eingebucht
			LOG_CODE_31 = 0x0410000B,	//	Aus WAN ausgebucht
			LOG_CODE_32 = 0x0410000C,	//	Von WAN getrennt
			LOG_CODE_33 = 0x0410000D,	//	Störung der WAN - Schnittstelle
			LOG_CODE_34 = 0x0440000A,	//	CSD - Connect
			LOG_CODE_35 = 0x0440000B,	//	CSD - Disconnect(from here)
			LOG_CODE_36 = 0x0440000C,	//	CSD - Disconnect(from remote)
			LOG_CODE_37 = 0x4140000A,	//	Lokaler IP - Zugang erfolgt
			LOG_CODE_38 = 0x4140000B,	//	Lokaler IP - Zugang beendet
			LOG_CODE_39 = 0x4140000C,	//	Lokaler IP - Zugang getrennt
			LOG_CODE_40 = 0x4140000D,	//	Lokaler IP - Zugang abgelehnt
			LOG_CODE_41 = 0x4140000E,	//	Lokaler IP - Zugang verloren
			LOG_CODE_42 = 0x4280000A,	//	Verbindung erfolgt, Beginn von SML - Datenverkehr
			LOG_CODE_43 = 0x4280000B,	//	Verbindung beendet, Ende von SML - Datenverkehr
			LOG_CODE_44 = 0x4280000C,	//	Fehlerhafte SML - Nachricht
			LOG_CODE_45 = 0x4840000A,	//	WAN - Dienst - Zugang erfolgt
			LOG_CODE_46 = 0x4840000C,	//	WAN - Dienst - Zugang getrennt
			LOG_CODE_47 = 0x4840000B,	//	WAN - Dienst - Zugang beendet
			LOG_CODE_48 = 0x4840000D,	//	WAN - Dienst - Zugang abgelehnt
			LOG_CODE_49 = 0x4840000E,	//	WAN - Dienst - Zugang verloren
			LOG_CODE_50 = 0x4840000F,	//	Kommando zum PLC - NetzNeuaufbau erhalten.
			LOG_CODE_51 = 0x48400010,	//	Kommando zum PLC - NetzNeuaufbau ausgeführt.
			LOG_CODE_52 = 0x48400011,	//	Kommando zum Abmeldung von PLC - Geräten erhalten.
			LOG_CODE_53 = 0x48400012,	//	Kommando zum Abmeldung von PLC - Geräten ausgeführt.
			LOG_CODE_54 = 0x48400013,	//	Warnung vor Überlastmodus
			LOG_CODE_55 = 0x48400014,	//	Gerät im Überlastmodus
			LOG_CODE_56 = 0x48400015,	//	Überlastmodus beendet
			LOG_CODE_57 = 0x48400016,	//	ZDUE - PLC - MUC angemeldet
			LOG_CODE_58 = 0x48400017,	//	Datensynchronisation abgebrochen
			LOG_CODE_59 = 0x48400018,	//	Datensynchronisation fehlgeschlagen
			LOG_CODE_60 = 0x48400019,	//	Datensynchronisation erfolgreich
			LOG_CODE_61 = 0x4970000A,	//	IP-T - Zugang erfolgt
			LOG_CODE_62 = 0x4970000B,	//	IP-T - Zugang beendet
			LOG_CODE_63 = 0x4970000C,	//	IP-T - Zugang getrennt, Verbindung von IP-T - Master getrennt
			LOG_CODE_64 = 0x4970000D,	//	IP-T - Zugang abgelehnt, Verbindung von IP-T - Master abgelehnt
			LOG_CODE_65 = 0x4970000E,	//	IP-T - Zugang verloren, Verbindung unerwartet abgebrochen
			LOG_CODE_66 = 0x4A70000A,	//	NTP - Zugang erfolgt, Verbindung zu Dienst ist aufgebaut(NTP)
			LOG_CODE_67 = 0x4A70000B,	//	NTP - Zugang beendet
			LOG_CODE_68 = 0x4A70000C,	//	NTP - Zugang getrennt
			LOG_CODE_69 = 0x4A70000D,	//	NTP - Zugang abgelehnt
			LOG_CODE_70 = 0x4A70000E,	//	NTP - Zugang verloren
			LOG_CODE_71 = 0x4A70000F,	//	Zeit wurde bei NTP - Sync um > 60sec zurückgestellt
			LOG_CODE_72 = 0x4A800000,	//	NTP Sync Zusatzinfos
		};

		/**
		 * These are OBIS codes
		 */
		enum event_sources : std::uint64_t
		{
			EVT_SOURCE_01 = 0x8101000000FF,	//	ETH(Erweiterungs - Schnittstelle)
			EVT_SOURCE_02 = 0x8102000000FF,	//	ETH(Kunden - Schnittstelle)
			EVT_SOURCE_03 = 0x8103000000FF,	//	RS232(Service - Schnittstelle)
			EVT_SOURCE_04 = 0x8104000000FF,	//	ETH(WAN - Schnittstelle)
			EVT_SOURCE_05 = 0x8105000000FF,	//	eHZ - Schnittstelle
			EVT_SOURCE_06 = 0x8106000000FF,	//	W-MBus - Schnittstelle
			EVT_SOURCE_07 = 0x8141000000FF,	//	IP(Erweiterungs - Schnittstelle)
			EVT_SOURCE_08 = 0x814200000001,	//	SML(Erweiterungs--Schnittstelle)
			EVT_SOURCE_09 = 0x814200000002,	//	SML(Kunden - Schnittstelle)
			EVT_SOURCE_10 = 0x814200000003,	//	SML(Service - Schnittstelle)
			EVT_SOURCE_11 = 0x814200000004,	//	SML(WAN - Schnittstelle)
			EVT_SOURCE_12 = 0x814200000005,	//	SML(eHZ - Schnittstelle)
			EVT_SOURCE_13 = 0x814200000006,	//	SML(W - MBUS - Schnittstelle)
			EVT_SOURCE_14 = 0x8145000000FF,	//	Push(SML)
			EVT_SOURCE_15 = 0x8146000000FF,	//	Push(IPT - Quelle)
			EVT_SOURCE_16 = 0x8147000000FF,	//	Push(IPT - Senke)
			EVT_SOURCE_17 = 0x814800000001,	//	WAN(DHCP)
			EVT_SOURCE_18 = 0x814800000002,	//	WAN(IP)
			EVT_SOURCE_19 = 0x814800000003,	//	WAN(PPPoE)
			EVT_SOURCE_20 = 0x814900000001,	//	WAN(IPT - Cntrl)
			EVT_SOURCE_21 = 0x814900000002,	//	WAN(IPT)
			EVT_SOURCE_22 = 0x814A000000FF,	//	WAN(NTP)
			EVT_SOURCE_23 = 0x818100000001,	//	Betriebslogbuch(Zykl.Einträge)
			EVT_SOURCE_24 = 0x818100000002,	//	SCM(Interne MUC - SW - Funktion)
			EVT_SOURCE_25 = 0x818100000003,	//	UPDATE(Interne MUC - SW - Funktion)
			EVT_SOURCE_26 = 0x818100000004,	//	SMLC(Interne MUC - SW - Funktion)
			EVT_SOURCE_27 = 0x818100000005,	//	OHDL_MUCCFG(Interne MUC - SW - Funktion)
			EVT_SOURCE_28 = 0x818100000006,	//	OHDL_SMLDL(Interne MUC - SW - Funktion)
			EVT_SOURCE_29 = 0x818100000007,	//	AUTHC(Interne MUC - SW - Funktion)
			EVT_SOURCE_30 = 0x818100000008,	//	BSZ(Interne MUC - SW - Funktion)
			EVT_SOURCE_31 = 0x818100000009,	//	DATACOLL(Interne MUC - SW - Funktion)
			EVT_SOURCE_32 = 0x81810000000A,	//	SHDL(Interne MUC - SW - Funktion)
			EVT_SOURCE_33 = 0x81810000000B,	//	GPRS(Interne MUC - SW - Funktion)
			EVT_SOURCE_34 = 0x81810000000C,	//	LEDIO(Interne MUC - SW - Funktion)
			EVT_SOURCE_35 = 0x81810000000D,	//	LISTC(Interne MUC - SW - Funktion)
			EVT_SOURCE_36 = 0x81810000000E,	//	SYNC_MUC(Interne MUC - SW - Funktion)
			EVT_SOURCE_37 = 0x81810000000F,	//	SYNC_MUCDC(Interne MUC - SW - Funktion)
			EVT_SOURCE_38 = 0x818100000010,	//	EHZIF(Interne MUC - SW - Funktion)
			EVT_SOURCE_39 = 0x818100000011,	//	USERIF(Interne MUC - SW - Funktion)
			EVT_SOURCE_40 = 0x818100000012,	//	WMBUS(Interne MUC - SW - Funktion)
			EVT_SOURCE_41 = 0x818100000013,	//	WANGSM(Interne MUC - SW - Funktion)
			EVT_SOURCE_42 = 0x818100000014,	//	WANPLC(Interne MUC - SW - Funktion)
			//
			EVT_SOURCE_43 = 0x8146000012FF,	//	Load switch module status
		};

	}	//	sml
}

#endif