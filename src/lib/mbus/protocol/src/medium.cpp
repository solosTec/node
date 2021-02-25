/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/medium.h>

namespace smf
{
	namespace mbus
	{
		const char* get_medium_name(device_type m)
		{
			switch (m)
			{
			case device_type::OTHER: return "other";
			case device_type::OIL: return "Oil";
			case device_type::ELECTRICITY: return "Electricity";
			case device_type::GAS: return "Gas";
			case device_type::HEAT: return "Heat";
			case device_type::STEAM: return "Steam";
			case device_type::WARM_WATER: return "Warm Water (30C...90C)";
			case device_type::WATER: return "Water";
			case device_type::HEAT_COST_ALLOCATOR: return "Heat Cost Allocator";
			case device_type::COMPRESSED_AIR: return "Compressed Air";
			case device_type::CLM_OUTLET: return "Cooling load meter (Volume measured at return temperature: outlet)";
			case device_type::CLM_INLET: return "Cooling load meter (Volume measured at flow temperature: inlet)";
			case device_type::HEAT_INLET: return "Heat (Volume measured at flow temperature: inlet)";
			case device_type::HEAT_COOLING_LOAD_METER: return "Heat / Cooling load meter";
			case device_type::BUS_SYSTEM_COMPONENT: return "Bus / System component";
			case device_type::UNKNOWN_MEDIUM: return "Unknwon medium";
				// 0x10 to 0x14 reserved
			case device_type::HOT_WATER: return "Hot water (>=90C)";
			case device_type::COLD_WATER: return "Cold water";
			case device_type::DUAL_REGISTER_WATER_METER: return "Dual register (hot/cold) Water Meter";
			case device_type::PRESSURE: return "Pressure meter";
			case device_type::AD_CONVERTER: return "A/D Converter";
			case device_type::SMOKE_DETECTOR: return "Smoke detector";
			case device_type::ROOM_SENSOR: return "Room sensor  (e.g. temperature or humidity)";
			case device_type::GAS_DETECTOR: return "Gas detector";
				// 0x1A to 0x20 reserved
			case device_type::VALVE: return "Reserved for valve";
				// 0x22 to 0xFF reserved
			case device_type::DISPLAY: return "Display device (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::MUC: return "OMS MUC (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::REPEATER_UNIDIRECTIONAL: return "OMS unidirectional repeater (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::REPEATER_BIDIRECTIONAL: return "OMS bidirectional repeater (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::SM_ELECTRICITY: return "Smart Metering Electricity Meter (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::SM_GAS: return "Smart Metering Gas Meter (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::SM_HEAT: return "Smart Metering Heat Meter (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::SM_WATER: return "Smart Metering Water Meter (OMS Vol.2 Issue 2.0.02009-07-20)";
			case device_type::SM_HCALLOC: return "Smart Metering Heat Cost Allocator (OMS Vol.2 Issue 2.0.02009-07-20)";
			default:
				break;
			}
			return "reserved";
		}

	}
}


