/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/obis_db.h>

namespace node
{
	namespace sml
	{
		bool is_profile(obis const& code)
		{
			return code.is_matching(0x81, 0x81, 0xC7, 0x86);
		}

		std::string get_name(obis const& code)
		{
			switch (code.to_uint64()) {

			case CODE_VERSION:		return "VERSION";		//
			case CODE_FILE_NAME:	return "FILE_NAME";		//
			case CODE_MSG_COUNTER:	return "MSG_COUNTER";	// [u32]
			case CODE_LAST_MSG:		return "LAST_MSG";		// [u32]
			case CODE_MSG_NUMBER:	return "MSG_NUMBER";	//
			case CODE_BLOCK_NUMBER:	return "BLOCK_NUMBER";	//
			case CODE_BINARY_DATA:	return "BINARY_DATA";	//

			case CODE_DATA_MANUFACTURER:	return "DATA_MANUFACTURER";
			case 0x8181C78205FF:	return "public-key";
			case 0x8181C78610FF:	return "profile-1min";
			case 0x8181C78611FF:	return "profile-15min";
			case 0x8181C78612FF:	return "profile-1h";
			case 0x8181C78613FF:	return "profile-1d";
			case 0x8181C78614FF:	return "profile-last-2h";
			case 0x8181C78615FF:	return "profile-last-week";
			case 0x8181C78616FF:	return "profile-1month";
			case 0x8181C78617FF:	return "profile-1y";
			case 0x8181C78618FF:	return "profile-initial";

			case 0x8181C78A83FF:	return "encode-profile";	//	OBIS_PROFILE

			case 0x8181C789E1FF:	return "class-operation-log";	//	CLASS_OP_LOG
			case 0x8181C789E2FF:	return "class-event";			//	CLASS_EVENT

			case CODE_ROOT_FILE_TRANSFER:		return "ROOT_FILE_TRANSFER";	
			case CODE_DATA_FIRMWARE:			return "DATA_FIRMWARE";
			case CODE_DATA_FILENAME: 			return "DATA_FILENAME";	
			case CODE_DATA_FILENAME_INDIRECT: 	return "DATA_FILENAME_INDIRECT";
			case CODE_DATA_APPLICATION: 		return "DATA_APPLICATION";
			case CODE_DATA_APPLICATION_INDIRECT: 	return "DATA_APPLICATION_INDIRECT";

			case CODE_REBOOT:	return "REBOOT";	//	REBOOT

			case 0x8101000000FF:	return "log-source-ETH_AUX";	//	OBIS_LOG_SOURCE_ETH_AUX
			case 0x8102000000FF:	return "log-source-ETH-CUSTOM";	//	OBIS_LOG_SOURCE_ETH_CUSTOM
			case 0x8103000000FF:	return "log-source-RS232";		//	OBIS_LOG_SOURCE_RS232
			case 0x8104000000FF:	return "log-source-WAN";		//	OBIS_LOG_SOURCE_ETH
			case 0x8105000000FF:	return "log-source-eHZ";		//	OBIS_LOG_SOURCE_eHZ 
			case 0x8106000000FF:	return "log-source-wMBus";		//	OBIS_LOG_SOURCE_wMBUS

			case 0x8141000000FF:	return "log-source-IP";			//	OBIS_LOG_SOURCE_IP
			case 0x814200000001:	return "log-source-SML-ext";	//	OBIS_LOG_SOURCE_SML_EXT
			case 0x814200000002:	return "log-source-SML_custom";	//else if (OBIS_LOG_SOURCE_SML_CUSTOM == code)			
			case 0x814200000003:	return "log-source-SML-service";	//else if (OBIS_LOG_SOURCE_SML_SERVICE == code)			
			case CODE_LOG_SOURCE_SML_WAN:	return "LOG_SOURCE_SML_WAN";	//else if (OBIS_LOG_SOURCE_SML_WAN == code)				
			case CODE_LOG_SOURCE_SML_eHZ:	return "LOG_SOURCE_SML_eHZ";
			case CODE_LOG_SOURCE_SML_wMBUS:	return "LOG_SOURCE_SML_wMBUS";
			case CODE_LOG_SOURCE_PUSH_SML:	return "LOG_SOURCE_PUSH_SML";
			case CODE_LOG_SOURCE_PUSH_IPT_SOURCE:	return "LOG_SOURCE_PUSH_IPT_SOURCE";
			case CODE_LOG_SOURCE_PUSH_IPT_SINK:	return "LOG_SOURCE_PUSH_IPT_SINK";
			case CODE_LOG_SOURCE_WAN_DHCP:	return "LOG_SOURCE_WAN_DHCP";
			case CODE_LOG_SOURCE_WAN_IP:	return "LOG_SOURCE_WAN_IP";
			case CODE_LOG_SOURCE_WAN_PPPoE:	return "LOG_SOURCE_WAN_PPPoE";
			case CODE_LOG_SOURCE_WAN_IPT_CONTROLLER:	return "LOG_SOURCE_WAN_IPT_CONTROLLER";	
			case CODE_LOG_SOURCE_WAN_IPT:	return "LOG_SOURCE_WAN_IPT";
			case CODE_LOG_SOURCE_WAN_NTP:	return "LOG_SOURCE_WAN_NTP";

			case CODE_CLASS_MBUS:	return "CLASS_MBUS";
			case CODE_CLASS_MBUS_RO_INTERVAL:	return "CLASS_MBUS_RO_INTERVAL";
			case CODE_CLASS_MBUS_SEARCH_INTERVAL:	return "CLASS_MBUS_SEARCH_INTERVAL";
			case CODE_CLASS_MBUS_SEARCH_DEVICE:	return "CLASS_MBUS_SEARCH_DEVICE";
			case CODE_CLASS_MBUS_AUTO_ACTICATE:	return "CLASS_MBUS_AUTO_ACTICATE";
			case CODE_CLASS_MBUS_BITRATE:	return "CLASS_MBUS_BITRATE";

			case CODE_W_MBUS_PROTOCOL:	return "W_MBUS_PROTOCOL";
			case CODE_W_MBUS_MODE_S: return "W_MBUS_MODE_S";
			case CODE_W_MBUS_MODE_T: return "W_MBUS_MODE_T";

			case CODE_W_MBUS_REBOOT: return "W_MBUS_REBOOT";
			case CODE_W_MBUS_POWER: return "W_MBUS_POWER";
			case CODE_W_MBUS_INSTALL_MODE: return "W_MBUS_INSTALL_MODE";

			//
			//	root elements
			//
			case CODE_ROOT_NTP:						return "ROOT_NTP";
			case CODE_ROOT_DEVICE_IDENT:			return "ROOT_DEVICE_IDENT";
			case CODE_DEVICE_CLASS:					return "DEVICE_CLASS";
			case CODE_SERVER_ID:					return "SERVER_ID";
			case CODE_ROOT_FIRMWARE:				return "ROOT_FIRMWARE";
			case CODE_DEVICE_KERNEL:				return "DEVICE_KERNEL";
			case CODE_DEVICE_ACTIVATED:				return "DEVICE_ACTIVATED";
			case CODE_ROOT_ACCESS_RIGHTS:			return "ROOT_ACCESS_RIGHTS";
			case CODE_ROOT_CUSTOM_INTERFACE:		return "ROOT_CUSTOM_INTERFACE";
			case CODE_ROOT_CUSTOM_PARAM:			return "ROOT_CUSTOM_PARAM";
			case CODE_ROOT_WAN:						return "ROOT_WAN";
			case CODE_ROOT_GSM:						return "ROOT_GSM";
			case CODE_ROOT_IPT_STATE:				return "ROOT_IPT_STATE";
			case CODE_ROOT_IPT_PARAM:				return "ROOT_IPT_PARAM";
			case CODE_PEER_ADDRESS_WANGSM:			return "EER_ADDRESS_WANGSM";
			case CODE_PEER_ADDRESS:					return "PEER_ADDRESS";

			case CODE_ROOT_GPRS_PARAM:				return "ROOT_GPRS_PARAM";
			case CODE_ROOT_W_MBUS_STATUS:			return "ROOT_W_MBUS_STATUS";
			case CODE_ROOT_LAN_DSL:					return "ROOT_LAN_DSL";
			case CODE_ROOT_MEMORY_USAGE:			return "ROOT_MEMORY_USAGE";
			case CODE_ROOT_MEMORY_MIRROR:			return "ROOT_MEMORY_MIRROR";
			case CODE_ROOT_MEMORY_TMP:				return "ROOT_MEMORY_TMP";
			case CODE_ROOT_DEVICE_TIME:				return "ROOT_DEVICE_TIME";
			case CODE_ROOT_ACTIVE_DEVICES:			return "ROOT_ACTIVE_DEVICES";
			case CODE_ROOT_NEW_DEVICES:				return "ROOT_NEW_DEVICES";
			case CODE_ROOT_INVISIBLE_DEVICES:		return "ROOT_INVISIBLE_DEVICES";
			case CODE_ROOT_DEVICE_INFO:				return "ROOT_DEVICE_INFO";
			case CODE_ROOT_VISIBLE_DEVICES:			return "OOT_VISIBLE_DEVICES";
			case CODE_ROOT_SENSOR_PARAMS:			return "ROOT_SENSOR_PARAMS";
			case CODE_ROOT_DATA_COLLECTOR:			return "ROOT_DATA_COLLECTOR";
			case CODE_STORAGE_TIME_SHIFT:			return "STORAGE_TIME_SHIFT";	//	?

			case CODE_TCP_WAIT_TO_RECONNECT:		return "TCP_WAIT_TO_RECONNECT";
			case CODE_TCP_CONNECT_RETRIES:			return "TCP_CONNECT_RETRIES";

			case CODE_IF_1107:						return "IF_1107";
			case CODE_IF_1107_ACTIVE:				return "IF_1107_ACTIVE";
			case CODE_IF_1107_LOOP_TIME:			return "IF_1107_LOOP_TIME";
			case CODE_IF_1107_RETRIES:				return "IF_1107_RETRIES";
			case CODE_IF_1107_MIN_TIMEOUT:			return "IF_1107_MIN_TIMEOUT";
			case CODE_IF_1107_MAX_TIMEOUT:			return "IF_1107_MAX_TIMEOUT";
			case CODE_IF_1107_MAX_DATA_RATE:		return "IF_1107_MAX_DATA_RATE";
			case CODE_IF_1107_RS485:				return "IF_1107_RS485";
			case CODE_IF_1107_PROTOCOL_MODE:		return "IF_1107_PROTOCOL_MODE";
			case CODE_IF_1107_METER_LIST:			return "IF_1107_METER_LIST";
			case CODE_IF_1107_AUTO_ACTIVATION:		return "IF_1107_AUTO_ACTIVATION";
			case CODE_IF_1107_TIME_GRID:			return "IF_1107_TIME_GRID";
			case CODE_IF_1107_TIME_SYNC:			return "IF_1107_TIME_SYNC";
			case CODE_IF_1107_MAX_VARIATION:		return "IF_1107_MAX_VARIATION";

			default:
				break;
			}

			if (OBIS_PUSH_OPERATIONS == code)		return "root-push-ops";	//	7.3.1.26 Datenstruktur zum Transport der Eigenschaften von Push-Vorgängen. 
			else if (OBIS_PUSH_INTERVAL == code)		return "push-interval";	//	in seconds
			else if (OBIS_PUSH_DELAY == code)			return "push-delay";	//	in seconds
			else if (OBIS_PUSH_SOURCE == code)			return "push-source";	//	options are PUSH_SOURCE_PROFILE, PUSH_SOURCE_INSTALL and PUSH_SOURCE_SENSOR_LIST
			else if (OBIS_PUSH_SOURCE_PROFILE == code)	return "push-source-profile";	//!< new meter/sensor data
			else if (OBIS_PUSH_SOURCE_INSTALL == code)	return "push-source-install";	//!< configuration changed
			else if (OBIS_PUSH_SOURCE_SENSOR_LIST == code)		return "push-source-sensor-list";	//!< list of visible meters changed
			else if (OBIS_PUSH_SERVER_ID == code)		return "push-server-id";

			else if (OBIS_IF_LAN_DSL == code)	return "IF_LAN_DSL";
			else if (OBIS_CODE_IF_GSM == code)		return "IF_GSM";
			else if (OBIS_CODE_IF_GPRS == code)		return "IF_GPRS";
			else if (OBIS_CODE_IF_USER == code)		return "IF_USER";
			//else if (OBIS_CODE_IF_IPT == code)		return "IF_IPT";
			else if (OBIS_CODE_IF_EDL == code)		return "IF_EDL";
			else if (OBIS_IF_wMBUS == code)	return "IF-wireless-mbus";
			else if (OBIS_CODE_IF_PLC == code)		return "IF_PLC";
			//else if (OBIS_CODE_IF_SyM2 == code)		return "IF_SyM2";
			else if (OBIS_IF_1107 == code)		return "IF-IEC-62505-21";	//	wired M-Bus

			else if (OBIS_W_MBUS_ADAPTER_MANUFACTURER == code)		return "W_MBUS_ADAPTER_MANUFACTURER";
			else if (OBIS_W_MBUS_ADAPTER_ID == code)				return "W_MBUS_ADAPTER_ID";
			else if (OBIS_W_MBUS_FIRMWARE == code)					return "W_MBUS_FIRMWARE";
			else if (OBIS_W_MBUS_HARDWARE == code)					return "W_MBUS_HARDWARE";

			else if (code == OBIS_CLASS_STATUS)			return "class-status";

			else if (OBIS_ACT_SENSOR_TIME == code)		return "act-sensor-time";
			else if (OBIS_SERIAL_NR == code)			return "serial-number-I";
			else if (OBIS_SERIAL_NR_SECOND == code)		return "serial-number-II";
			else if (OBIS_FABRICATION_NR == code)		return "fabrication-number";
			else if (OBIS_POWER_OUTAGES == code)		return "power-outages";
			else if (OBIS_MBUS_STATE == code)			return "status-EN13757-3";
			else if (OBIS_DATE_TIME_PARAMETERISATION == code)	return "date-time-parameterisation";
			else if (OBIS_SECONDS_INDEX == code)		return "seconds-index";
			else if (OBIS_HARDWARE_TYPE == code)		return "hardware-type";

			//	Identifikationsnummer 1.1 - comes as unsigned int with 3 bytes (this is the server ID)
			else if (code == OBIS_SERVER_ID_1_1)		return "identifier-1-1";
			else if (code == OBIS_SERVER_ID_1_2)		return "identifier-1-2";
			else if (code == OBIS_SERVER_ID_1_3)		return "identifier-1-3";
			else if (code == OBIS_SERVER_ID_1_4)		return "identifier-1-4";
			else if (code == OBIS_DEVICE_ID)			return "device-id";
			else if (OBIS_SOFTWARE_ID == code)			return "software_id";

			else if (code == OBIS_CURRENT_UTC)			return "readout-utc";
			else if (code == OBIS_PUSH_SERVICE)			return "push-service";
			else if (code == OBIS_PUSH_SERVICE_IPT)		return "push-service:IPT";
			else if (code == OBIS_PUSH_SERVICE_SML)		return "push-service:SML";
			else if (code == OBIS_DATA_COLLECTOR_OBIS)	return "list-of-OBIS-codes";
			else if (code == OBIS_DATA_MANUFACTURER)	return "manufacturer";
			else if (code == OBIS_DATA_PUBLIC_KEY)		return "public-key";
			else if (code == OBIS_DATA_IP_ADDRESS)		return "IP-address";
			else if (code == OBIS_DATA_AES_KEY)			return "AES-key";
			else if (code == OBIS_DATA_USER_NAME)		return "user-name";
			else if (code == OBIS_DATA_USER_PWD)		return "user-pwd";
			else if (code == OBIS_SET_ACTIVATE_FW)		return "SET-activate-FW";
			else if (code == OBIS_SET_START_FW_UPDATE)	return "SET-update-FW";
			else if (code == OBIS_SET_DISPATCH_FW_UPDATE)	return "SET-dispatch-FW";

			else if (code == OBIS_REG_POS_AE_NO_TARIFF)			return "pos-act-energy-no-tariff";
			else if (code == OBIS_REG_POS_AE_T1)				return "pos-act-energy-tariff-1";
			else if (code == OBIS_REG_POS_AE_T2)				return "pos-act-energy-tariff-2";
			else if (code == OBIS_REG_NEG_AE_NO_TARIFF)			return "neg-act-energy-no-tariff";
			else if (code == OBIS_REG_NEG_AE_T1)				return "neg-act-energy-tariff-1";
			else if (code == OBIS_REG_NEG_AE_T2)				return "neg-act-energy-tariff-2";
			else if (code == OBIS_REG_CUR_POS_AE)				return "pos-act-energy-current";
			else if (code == OBIS_REG_CUR_AP)					return "act-power-current";

			//																				//		const static obis	OBIS_CLASS_OP_LOG_EVENT(0x81, 0x81, 0xC7, 0x89, 0xE2, 0xFF);	//	Ereignis (uint32)
			//const static obis	OBIS_CLASS_OP_LOG_PEER_ADDRESS(0x81, 0x81, 0x00, 0x00, 0x00, 0xFF);	//	Peer Adresse (octet string)
			//const static obis	OBIS_CLASS_OP_MONITORING_STATUS(0x00, 0x80, 0x80, 0x11, 0x13, 0x01);	//	Überwachungsstatus (uint8) - tatsächlich i32
			//																							/**
			//																							*	Statuswort (per Schreiben ist das Rücksetzen ausgewählter Statusbits) Unsigned64
			//																							*/
			else if (OBIS_CLASS_OP_LOG_STATUS_WORD == code)			return "op-log-status-word";
			else if (OBIS_CLASS_OP_LOG_FIELD_STRENGTH == code)		return "op-log-field-strength";
			else if (OBIS_CLASS_OP_LOG_CELL == code)				return "op-log-cell-code";
			else if (OBIS_CLASS_OP_LOG_AREA_CODE == code)			return "op-log-area-code";
			else if (OBIS_CLASS_OP_LOG_PROVIDER == code)			return "op-log-provider";

			else if (code == OBIS_CLASS_OP_LSM_STATUS)					return "LSM-status";
			else if (code == OBIS_CLASS_OP_LSM_ACTOR_ID)				return "LSM-actor-id";
			else if (code == OBIS_CLASS_OP_LSM_CONNECT)					return "LSM-connection-status";
			else if (code == OBIS_CLASS_OP_LSM_SWITCH)					return "LSM-switch";
			else if (code == OBIS_CLASS_OP_LSM_FEEDBACK)				return "LSM-feedback";
			else if (code == OBIS_CLASS_OP_LSM_LOAD)					return "LSM-load-count";
			else if (code == OBIS_CLASS_OP_LSM_POWER)					return "LSM-total-power";
			else if (code == OBIS_CLASS_OP_LSM_VERSION)					return "LSM-version";
			else if (code == OBIS_CLASS_OP_LSM_TYPE)					return "LSM-type";
			else if (code == OBIS_CLASS_OP_LSM_ACTIVE_RULESET)			return "LSM-active-ruleset";
			else if (code == OBIS_CLASS_OP_LSM_PASSIVE_RULESET)			return "LSM-passive-ruleset";
			else if (code == OBIS_CLASS_OP_LSM_JOB)						return "LSM-job-name";
			else if (code == OBIS_CLASS_OP_LSM_POSITION)				return "LSM-position";

			else if (OBIS_LIST_CURRENT_DATA_RECORD == code)				return "current-data-record";


			//
			//	attention codes
			//
			return get_attention_name(code);
		}

		std::string get_attention_name(obis const& code)
		{
			if (OBIS_ATTENTION_UNKNOWN_ERROR == code)		return "UNKNOWN ERROR";
			else if (OBIS_ATTENTION_UNKNOWN_SML_ID == code) return "UNKNOWN SML ID";
			else if (OBIS_ATTENTION_NOT_AUTHORIZED == code) return "NOT AUTHORIZED";
			else if (OBIS_ATTENTION_NO_SERVER_ID == code) 	return "NO SERVER ID";
			else if (OBIS_ATTENTION_NO_REQ_FIELD == code)	return "NO REQ FIELD";
			else if (OBIS_ATTENTION_CANNOT_WRITE == code)	return "CANNOT WRITE";
			else if (OBIS_ATTENTION_CANNOT_READ == code)	return "CANNOT READ";
			else if (OBIS_ATTENTION_COMM_ERROR == code)		return "COMM ERROR";
			else if (OBIS_ATTENTION_PARSER_ERROR == code)	return "PARSER ERROR";
			else if (OBIS_ATTENTION_OUT_OF_RANGE == code)	return "OUT OF RANGE";
			else if (OBIS_ATTENTION_NOT_EXECUTED == code)	return "NOT EXECUTED";
			else if (OBIS_ATTENTION_INVALID_CRC == code)	return "INVALID CRC";
			else if (OBIS_ATTENTION_NO_BROADCAST == code)	return "NO BROADCAST";
			else if (OBIS_ATTENTION_UNEXPECTED_MSG == code) return "UNEXPECTED MSG";
			else if (OBIS_ATTENTION_UNKNOWN_OBIS_CODE == code)	return "UNKNOWN OBIS CODE";
			else if (OBIS_ATTENTION_UNSUPPORTED_DATA_TYPE == code)	return "UNSUPPORTED DATA TYPE";
			else if (OBIS_ATTENTION_ELEMENT_NOT_OPTIONAL == code)	return "NOT OPTIONAL";
			//	requested profile has no entry
			else if (OBIS_ATTENTION_NO_ENTRIES == code)				return "NO ENTRY";
			else if (OBIS_ATTENTION_END_LIMIT_BEFORE_START == code)	return "END LIMIT BEFORE START";
			else if (OBIS_ATTENTION_NO_ENTRIES_IN_RANGE == code)	return "NO ENTRIES IN RANGE";
			else if (OBIS_ATTENTION_MISSING_CLOSE_MSG == code)		return "MISSING CLOSE MSG";
			else if (OBIS_ATTENTION_OK == code)						return "OK";
			else if (OBIS_ATTENTION_JOB_IS_RUNNINNG == code)		return "JOB IS RUNNINNG";
			return code.to_str();
		}

		const char* get_LSM_event_name(std::uint32_t evt)
		{
			switch (evt & 0xFFFFFF)
			{
			case 0x810120:	return "switching operation";
			case 0x810101:	return "Regelsatzänderung";
			case 0x810102:	return "job received";
			case 0x810103:	return "Kontaktmatrix geändert";
			case 0x810104:	return "keine Kontaktmatrix parametriert";
			case 0x810105:	return "Objektkonfiguration geändert";
			case 0x810106:	return "Konfiguration in LSM programmiert";
			case 0x810107:	return "Konfiguration konnte nicht in LSM programmiert werden";
			case 0x810108:	return "Abweichende Konfiguration";
			case 0x810130:	return "Relaistest";
			case 0x810131:	return "Pegeländerung am MUC Eingangskontakt";
			case 0x810001:	return "MUC Zeitbasis unsicher";
			case 0x810111:	return "Objektschaltauftrag angenommen";
			case 0x810112:	return "Objektschaltauftrag abgelehnt";
			default:
				break;
			}
			return "unknown";
		}

		const char* get_profile_name(obis const& code)
		{
			switch (code.to_uint64()) {
			case CODE_PROFILE_1_MINUTE:	return "PROFILE-1-MINUTE";
			case CODE_PROFILE_15_MINUTE:	return "PROFILE-15-MINUTE";
			case CODE_PROFILE_60_MINUTE:	return "PROFILE-60-MINUTE";
			case CODE_PROFILE_24_HOUR:	return "PROFILE-24-HOUR";
			case CODE_PROFILE_LAST_2_HOURS:	return "PROFILE-LAST-2-HOURS";
			case CODE_PROFILE_LAST_WEEK:	return "PROFILE-LAST-WEEK";
			case CODE_PROFILE_1_MONTH:	return "PROFILE-1-MONTH";
			case CODE_PROFILE_1_YEAR:	return "PROFILE-1-YEAR";
			case CODE_PROFILE_INITIAL:	return "PROFILE-INITIAL";
			default:
				break;
			}
			return "not-a-profile";
		}

	}
}