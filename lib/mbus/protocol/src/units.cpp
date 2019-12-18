/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/units.h>

namespace node
{
	namespace mbus
	{
		const char* get_unit_name(std::uint8_t u)
		{
			switch (u)
			{
			case units::YEAR:							return "year";
			case units::MONTH:							return "month";
			case units::WEEK:							return "week";
			case units::DAY:							return "day";
			case units::HOUR:							return "hour";
			case units::MIN:							return "minute";
			case units::SECOND:							return "second";
			case units::DEGREE:							return "degree";
			case units::DEGREE_CELSIUS:					return "°C";
			case units::CURRENCY:						return "currency";
			case units::METER:							return "meter";
			case units::METRE_PER_SECOND:				return "meterPerSecond";
			case units::CUBIC_METRE:					return "m3";	//	return "cubicMeter";
			case units::CUBIC_METRE_CORRECTED:			return "cubicMeterCorrected";
			case units::CUBIC_METRE_PER_HOUR:			return "cubicMeterPerHour";
			case units::CUBIC_METRE_PER_HOUR_CORRECTED:return "cubicMeterPerHourCorrected";
			case units::CUBIC_METRE_PER_DAY:			return "cubicMeterPerDay";
			case units::CUBIC_METRE_PER_DAY_CORRECTED:	return "cubicMeterPerDayCorrected";
			case units::LITRE:							return "litre";
			case units::KILOGRAM:						return "kg";	//	return "kilogram";
			case units::NEWTON:							return "newton";
			case units::NEWTONMETRE:					return "newtonMetre";
			case units::UNIT_PASCAL:					return "pascal";
			case units::BAR:							return "bar";
			case units::JOULE:							return "joule";
			case units::JOULE_PER_HOUR:				return "joulePerHour";
			case units::WATT:							return "watt";
			case units::VOLT_AMPERE:					return "voltAmpere";
			case units::VAR:							return "var";
			case units::WATT_HOUR:						return "Wh";	//	"wattHour"
			case units::VOLT_AMPERE_HOUR:				return "voltAmpereHour";
			case units::VAR_HOUR:						return "varHour";
			case units::AMPERE:						return "ampere";
			case units::COULOMB:						return "coulomb";
			case units::VOLT:							return "volt";
			case units::VOLT_PER_METRE:				return "voltPerMetre";
			case units::FARAD:							return "farad";
			case units::OHM:							return "ohm";
			case units::OHM_METRE:						return "ohmMetre";
			case units::WEBER:							return "weber";
			case units::TESLA:							return "tesla";
			case units::AMPERE_PER_METRE:				return "amperePerMetre";
			case units::HENRY:							return "henry";
			case units::HERTZ:							return "Hz";	//	"hertz";
			case units::ACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE:		return "activeEnergyMeterConstantOrPulseValue";
			case units::REACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE:	return "reactiveEnergyMeterConstantOrPulseValue";
			case units::APPARENT_ENERGY_METER_CONSTANT_OR_PULSE_VALUE:	return "apparentEnergyMeterConstantOrPulseValue";
			case units::VOLT_SQUARED_HOURS:			return "voltSquaredHours";
			case units::AMPERE_SQUARED_HOURS:			return "ampereSquaredHours";
			case units::KILOGRAM_PER_SECOND:			return "kilogramPerSecond";
			case units::KELVIN:						return "kelvin";
			case units::VOLT_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE:		return "voltSquaredHourMeterConstantOrPulseValue";
			case units::AMPERE_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE:	return "ampereSquaredHourMeterConstantOrPulseValue";
			case units::METER_CONSTANT_OR_PULSE_VALUE:	return "meterConstantOrPulseValue";
			case units::PERCENTAGE:					return "percentage";
			case units::AMPERE_HOUR:					return "ampereHour";
			case units::ENERGY_PER_VOLUME:				return "energyPerVolume";
			case units::CALORIFIC_VALUE:				return "calorificValue";
			case units::MOLE_PERCENT:					return "molePercent";
			case units::MASS_DENSITY:					return "massDensity";
			case units::PASCAL_SECOND:					return "pascalSecond";
				 
			case units::CUBIC_METRE_PER_SECOND:		return "m³/s";
			case units::CUBIC_METRE_PER_MINUTE:		return "m³/min";
			case units::KILOGRAM_PER_HOUR:				return "kg/h";
			case units::CUBIC_FEET:					return "cft";
			case units::US_GALLON:						return "Impl. gal.";
			case units::US_GALLON_PER_MINUTE:			return "Impl. gal./min";
			case units::US_GALLON_PER_HOUR:			return "Impl. gal./h";
			case units::DEGREE_FAHRENHEIT:				return "°F";
				 
			case units::UNIT_RESERVED:					return "reserved";
			case units::OTHER_UNIT:					return "other";
			case units::COUNT:							return "counter";
			default:
				break;
			}

			//	not-a-unit
			return "not-a-unit";
		}
		
		const char* get_unit_name(units u) {
			return get_unit_name(static_cast<std::uint8_t>(u));
		}

	}
}


