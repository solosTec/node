/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_MEDIUM_H
#define SMF_MBUS_MEDIUM_H

#include <cstdint>
#include <string>

namespace smf
{
	namespace mbus	
	{
		enum class device_type : std::uint8_t {

			OTHER = 0x00,
			OIL = 0x01,	//!< Oil
			ELECTRICITY = 0x02,	//!< Electricity
			GAS = 0x03, //!< Gas
			HEAT = 0x04, //!< Heat
			STEAM = 0x05, //!< Steam
			WARM_WATER = 0x06, //!< Warm Water (30C...90C)
			WATER = 0x07, //!< Water
			HEAT_COST_ALLOCATOR = 0x08, //!< Heat Cost Allocator
			COMPRESSED_AIR = 0x09, //!< Compressed Air
			CLM_OUTLET = 0x0A, //!< Cooling load meter (Volume measured at return temperature: outlet)
			CLM_INLET = 0x0B, //!< Cooling load meter (Volume measured at flow temperature: inlet)
			HEAT_INLET = 0x0C, //!< Heat (Volume measured at flow temperature: inlet)
			HEAT_COOLING_LOAD_METER = 0x0D, //!< Heat / Cooling load meter
			BUS_SYSTEM_COMPONENT = 0x0E, //!< Bus / System component
			UNKNOWN_MEDIUM = 0x0F, //!< Unknwon medium

			// 0x10 to 0x13 reserved
			CALORIFIC_VALUE = 0x14,	//! Calorific value
			HOT_WATER = 0x15, //!< Hot water (>=90C)
			COLD_WATER = 0x16, //!< Cold water
			DUAL_REGISTER_WATER_METER = 0x17, //!< Dual register (hot/cold) Water Meter
			PRESSURE = 0x18, //!< Pressure meter
			AD_CONVERTER = 0x19, //!<	A/D Converter
			SMOKE_DETECTOR = 0x1A,	//!<	Room sensor  (e.g. temperature or humidity)
			ROOM_SENSOR = 0x1B,	//!<	Room sensor  (e.g. temperature or humidity)
			GAS_DETECTOR = 0x1C,	//!<	Gas detector

			// 0x10 to 0x1F reserved
			BREAKER = 0x22,	//!< (electricity)
			VALVE = 0x21,		//!< Valve (gas or water)

			// 0x22 to 0x24 reserved
			DISPLAY = 0x25, //!< Display device (OMS Vol.2 Issue 2.0.02009-07-20)

			//	Reserved for customer units 0x26 to 0x27
			WASTE_WATER = 0x28,	//!<	Waste water
			GARBAGE = 0x29,		//!<	Garbage	(not the band)
			CO2 = 0x2A,		//	Reserved for Carbon dioxide
			//	Reserved for environmental meter 0x2B to 0x2F
			MUC = 0x31, //!< OMS MUC (OMS Vol.2 Issue 2.0.02009-07-20)
			REPEATER_UNIDIRECTIONAL = 0x32, //!< OMS unidirectional repeater (OMS Vol.2 Issue 2.0.02009-07-20)
			REPEATER_BIDIRECTIONAL = 0x33, //!< OMS bidirectional repeater (OMS Vol.2 Issue 2.0.02009-07-20)

			//	Reserved for system devices 0x34 to 0x35
			RADIO_CONVERTER_SYSTEM = 0x36,	//!< A radio converter at system side operates as radio master like a wireless communication partner
			RADIO_CONVERTER_METER = 0x37,	//!< A Radio converter at meter side operates as radio slave like a RF-meter

			//	Reserved for system devices 0x38 to 0x3F
			SM_ELECTRICITY = 0x42, //!< Smart Metering Electricity Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			SM_GAS = 0x43, //!< Smart Metering Gas Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			SM_HEAT = 0x44, //!< Smart Metering Heat Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			SM_WATER = 0x47, //!< Smart Metering Water Meter (OMS Vol.2 Issue 2.0.02009-07-20)
			SM_HCALLOC = 0x48, //!< Smart Metering Heat Cost Allocator (OMS Vol.2 Issue 2.0.02009-07-20)
		};

		/**
		 * @return a the medium name (in english)
		 * @see http://www.m-bus.com/mbusdoc/md8.php
		 */
		const char* get_medium_name(device_type);

		/**
		 * get the enum value as unsigned int.
		 */
		constexpr std::uint8_t to_u8(device_type dev) {
			return static_cast<std::uint8_t>(dev);
		}
               
	}
}


#endif
