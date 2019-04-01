/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_MBUS_DEFS_H
#define NODE_SML_MBUS_DEFS_H

#include <string>
#include <cstdint>

namespace node
{
	namespace sml	
	{

		/**
		 *	@param id manufacturer id (contains 3 uppercase characters)
		 *	@return manufacturer id or 0 in case of error
		 *	@see http://www.dlms.com/flag/
		 */
		std::uint16_t encode_id(std::string const& id);

		/**
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return string with manufacturer ID (consisting of 3 characters)
		 */
		std::string decode(std::uint16_t);
		std::string decode(char, char);

		/**
		 * Read the manufacturer name from a hard coded table
		 * 
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return string with manufacturer name
		 */
		std::string get_manufacturer_name(std::uint16_t&);
		std::string get_manufacturer_name(char, char);		
		
		/**
		 * @return a the medium name (in english)
		 * @see http://www.m-bus.com/mbusdoc/md8.php
		 */
		std::string get_medium_name(std::uint8_t m);

	}	//	sml

	namespace mbus
	{
		//
		//	data link layer
		//

		//
		//	max frame size is 252 bytes
		//
		constexpr std::uint8_t FRAME_DATA_LENGTH = 0xFC;

		//
		// Frame start/stop bits
		//
		constexpr std::uint8_t FRAME_ACK_START	= 0xE5;	//	acknowledge receipt of transmissions
		constexpr std::uint8_t FRAME_SHORT_START = 0x10;
		constexpr std::uint8_t FRAME_CONTROL_START = 0x68;
		constexpr std::uint8_t FRAME_LONG_START = 0x68;
		constexpr std::uint8_t FRAME_STOP = 0x16;

		//
		//	C-Fields
		//	see https://oms-group.org/fileadmin/files/download4all/specification/Vol2/4.1.2/OMS-Spec_Vol2_Primary_v412.pdf
		//
		constexpr std::uint8_t CTRL_FIELD_SP_UD = 0x08;	//	
		constexpr std::uint8_t CTRL_FIELD_SND_NKE = 0x40;	//	
		constexpr std::uint8_t CTRL_FIELD_SND_UD2 = 0x43;	//	Send command with subsequent response (Send User Data - 2nd message type)
		constexpr std::uint8_t CTRL_FIELD_SND_NR = 0x44;	//	Send spontaneous/periodical application data without request (S1 mode)
		constexpr std::uint8_t CTRL_FIELD_REQ_UD1 = 0x5A;	//	Request User Data
		constexpr std::uint8_t CTRL_FIELD_REQ_UD2 = 0x5B;	//	Request User Data
		constexpr std::uint8_t CTRL_FIELD_SND_UD = 0x53;	//	
		//constexpr std::uint8_t CTRL_FIELD_IR = 0x00;		//	Send manually initiated installation data – Send installation request
		//constexpr std::uint8_t CTRL_FIELD_ACC_NR = 0x00;	//	No data – Provides opportunity to access the meter between two application transmissions
		//constexpr std::uint8_t CTRL_FIELD_ACC_DMD = 0x00;	//	Access demand to master in order to request new importat application data – Alerts
		//constexpr std::uint8_t CTRL_FIELD_ACC = 0x00;		//	Acknowledge the reception 
		//constexpr std::uint8_t CTRL_FIELD_RSP_UD = 0x00;	//	Response of application data after a request from master

		//
		//	CONTROL INFORMATION FIELD (CI-field)
		//
			
		enum ci_field : std::uint8_t {

			FIELD_CI_RESET = 0x50, //!< Reset(EN 13757-3)
			FIELD_CI_DATA_SEND = 0x51, //!<Data send (EN 13757-3)
			FIELD_CI_SLAVE_SELECT = 0x52,	//!< Slave select (EN 13757-3)
			FIELD_CI_APPLICATION_RESET = 0x53,	//!< Application reset or select (EN 13757-3).
			FIELD_CI_CMD_TO_DEVICE_SHORT = 0x5A,	//!< CMD to device with short header (OMS Vol.2 Issue 2.0.0/2009-07-20)								
			FIELD_CI_CMD_TO_DEVICE_LONG = 0x5B,	//!< CMD to device with long header (OMS Vol.2 Issue 2.0.0/2009-07-20)
			FIELD_CI_TIME_SYNC_1 = 0x6C,	//!< Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_TIME_SYNC_2 = 0x6D, //!<Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ERROR_SHORT = 0x6E, //!<Error from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ERROR_LONG = 0x6F, //!<Error from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ERROR = 0x70, //!<Report of application errors (EN 13757-3)
			FIELD_CI_ALARM = 0x71, //!<report of alarms (EN 13757-3)
			FIELD_CI_HEADER_LONG = 0x72, //!<12 byte header followed by variable format data (EN 13757-3)
			FIELD_CI_APL_ALARM_SHORT = 0x74, //!<Alarm from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ALARM_LONG = 0x75, //!<Alarm from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_HEADER_NO = 0x78, //!<Variable data format respond without header (EN 13757-3)
			FIELD_CI_HEADER_SHORT = 0x7A, //!<4 byte header followed by variable data format respond (EN 13757-3)
			FIELD_CI_LINK_TO_DEVICE_LONG = 0x80, //!<Link extension to device with long header (OMS Vol.2 Issue 2.0.02009-07-20)
			FIELD_CI_LINK_FROM_DEVICE_SHORT = 0x8A, //!<Link extension from device with short header
			FIELD_CI_LINK_FROM_DEVICE_LONG = 0x8B, //!<Link extension from device with long header(OMS Vol.2 Issue 2.0.0/2009-07-20)

//#if !OMS_V4_ENABLED
			
			FIELD_CI_EXT_DLL_I = 0x8CU,		//!< Additional Link Layer may be applied for Radio messages with or without Application Layer. 
			FIELD_CI_EXT_DLL_II = 0x8D, //!<Additional Link Layer may be applied for Radio messages with or without Application Layer. 
//#endif // OMS_V4_ENABLED

				
			FIELD_CI_MANU_NO = 0xA1, //!<Manufacture specific CI-Field with no header
			FIELD_CI_MANU_SHORT = 0xA2, //!<Manufacture specific CI-Field with short header
			FIELD_CI_MANU_LONG = 0xA3, //!<Manufacture specific CI-Field with long header
			FIELD_CI_MANU_SHORT_RF = 0xA4, //!<Manufacture specific CI-Field with short header (for RF-Test)
			FIELD_CI_NULL = 0xFF, //!<No CI-field transmitted.
		};

		//
		//	81 06 19 07 04 FF - transmision power
		//
		enum transmission_power : std::uint8_t {
			BASIC = 0,	//	default
			LOW = 1,	
			AVERAGE = 2,	//	medium
			STRONG = 3,		//	high
		};

		enum radio_protocol : std::uint8_t {

			MODE_S = 0,
			MODE_T = 1,
			MODE_C = 2,	//	alternating
			MODE_N = 3,	//	parallel
			MODE_S_SYNC = 4,	//	Used for synchronized messages in meters
			MODE_UNKNOWN = 5,
			RESERVED
		};

		enum device_type : std::uint8_t {
			DEV_TYPE_OTHER = 0x00,
			DEV_TYPE_OIL = 0x01,	//!< Oil
			DEV_TYPE_ELECTRICITY = 0x02U,	//!< Electricity
			DEV_TYPE_GAS = 0x03, //!< Gas
			DEV_TYPE_HEAT = 0x04, //!< Heat
			DEV_TYPE_STEAM = 0x05, //!< Steam
			DEV_TYPE_WARM_WATER = 0x06, //!< Warm Water (30C...90C)
			DEV_TYPE_WATER = 0x07, //!< Water
			DEV_TYPE_HEAT_COST_ALLOCATOR = 0x08, //!< Heat Cost Allocator
			DEV_TYPE_COMPRESSED_AIR = 0x09, //!< Compressed Air
			DEV_TYPE_CLM_OUTLET = 0x0A, //!< Cooling load meter (Volume measured at return temperature: outlet)
			DEV_TYPE_CLM_INLET = 0x0B, //!< Cooling load meter (Volume measured at flow temperature: inlet)
			DEV_TYPE_HEAT_INLET = 0x0C, //!< Heat (Volume measured at flow temperature: inlet)
			DEV_TYPE_HEAT_COOLING_LOAD_METER = 0x0D, //!< Heat / Cooling load meter
			DEV_TYPE_BUS_SYSTEM_COMPONENT = 0x0E, //!< Bus / System component
			DEV_TYPE_UNKNOWN_MEDIUM = 0x0F, //!< Unknwon medium
			// 0x10 to 0x14 reserved
			DEV_TYPE_HOT_WATER = 0x15, //!< Hot water (>=90C)
			DEV_TYPE_COLD_WATER = 0x16, //!< Cold water
			DEV_TYPE_DUAL_REGISTER_WATER_METER = 0x17, //!< Dual register (hot/cold) Water Meter
			DEV_TYPE_PRESSURE = 0x18, //!< Pressure
			DEV_TYPE_AD_CONVERTER = 0x19, //!< A/D Converter
			// 0x1A to 0x20 reserved
			DEV_TYPE_VALVE = 0x21, //!< Reserved for valve
			// 0x22 to 0xFF reserved
			DEV_TYPE_DISPLAY = 0x25, //!< Display device (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_MUC = 0x31, //!< OMS MUC (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_REPEATER_UNIDIRECTIONAL = 0x32, //!< OMS unidirectional repeater (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_REPEATER_BIDIRECTIONAL = 0x33, //!< OMS bidirectional repeater (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_SM_ELECTRICITY = 0x42, //!< Smart Metering Electricity Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_SM_GAS = 0x43, //!< Smart Metering Gas Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_SM_HEAT = 0x44, //!< Smart Metering Heat Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_SM_WATER = 0x47, //!< Smart Metering Water Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			DEV_TYPE_SM_HCALLOC = 0x48, //!< Smart Metering Heat Cost Allocator (OMS Vol.2 Issue 2.0.02009-07-20)
		};

		enum encryption_mode : std::uint8_t {
			ENCRYPTION_MODE_0 = 0U,
			ENCRYPTION_MODE_5 = 5U,
			ENCRYPTION_MODE_7 = 7U,
			ENCRYPTION_MODE_9 = 9U,
			ENCRYPTION_MODE_15 = 15U,
			ENCRYPTION_MODE_UNKNOWN = 255U
		};
	}

}	//	node


#endif	
