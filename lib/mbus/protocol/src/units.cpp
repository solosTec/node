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
			case static_cast<std::uint8_t>(units::YEAR):						return "year";
			case static_cast<std::uint8_t>(units::MONTH):						return "month";
			case static_cast<std::uint8_t>(units::WEEK):						return "week";
			case static_cast<std::uint8_t>(units::DAY):							return "day";
			case static_cast<std::uint8_t>(units::HOUR):						return "hour";
			case static_cast<std::uint8_t>(units::MIN):							return "minute";
			case static_cast<std::uint8_t>(units::SECOND):						return "second";
			case static_cast<std::uint8_t>(units::DEGREE):						return "degree";
			case static_cast<std::uint8_t>(units::DEGREE_CELSIUS):				return "°C";
			case static_cast<std::uint8_t>(units::CURRENCY):					return "currency";
			case static_cast<std::uint8_t>(units::METER):						return "meter";
			case static_cast<std::uint8_t>(units::METRE_PER_SECOND):			return "meterPerSecond";
			case static_cast<std::uint8_t>(units::CUBIC_METRE):					return "m3";	//	return "cubicMeter";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_CORRECTED):		return "cubicMeterCorrected";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_HOUR):		return "cubicMeterPerHour";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_HOUR_CORRECTED):return "cubicMeterPerHourCorrected";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_DAY):			return "cubicMeterPerDay";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_DAY_CORRECTED):	return "cubicMeterPerDayCorrected";
			case static_cast<std::uint8_t>(units::LITRE):						return "litre";
			case static_cast<std::uint8_t>(units::KILOGRAM):					return "kg";	//	return "kilogram";
			case static_cast<std::uint8_t>(units::NEWTON):						return "newton";
			case static_cast<std::uint8_t>(units::NEWTONMETRE):					return "newtonMetre";
			case static_cast<std::uint8_t>(units::UNIT_PASCAL):					return "pascal";
			case static_cast<std::uint8_t>(units::BAR):							return "bar";
			case static_cast<std::uint8_t>(units::JOULE):						return "joule";
			case static_cast<std::uint8_t>(units::JOULE_PER_HOUR):				return "joulePerHour";
			case static_cast<std::uint8_t>(units::WATT):						return "watt";
			case static_cast<std::uint8_t>(units::VOLT_AMPERE):					return "voltAmpere";
			case static_cast<std::uint8_t>(units::VAR):							return "var";
			case static_cast<std::uint8_t>(units::WATT_HOUR):					return "Wh";	//	"wattHour"
			case static_cast<std::uint8_t>(units::VOLT_AMPERE_HOUR):			return "voltAmpereHour";
			case static_cast<std::uint8_t>(units::VAR_HOUR):					return "varHour";
			case static_cast<std::uint8_t>(units::AMPERE):						return "ampere";
			case static_cast<std::uint8_t>(units::COULOMB):						return "coulomb";
			case static_cast<std::uint8_t>(units::VOLT):						return "volt";
			case static_cast<std::uint8_t>(units::VOLT_PER_METRE):				return "voltPerMetre";
			case static_cast<std::uint8_t>(units::FARAD):						return "farad";
			case static_cast<std::uint8_t>(units::OHM):							return "ohm";
			case static_cast<std::uint8_t>(units::OHM_METRE):					return "ohmMetre";
			case static_cast<std::uint8_t>(units::WEBER):						return "weber";
			case static_cast<std::uint8_t>(units::TESLA):						return "tesla";
			case static_cast<std::uint8_t>(units::AMPERE_PER_METRE):			return "amperePerMetre";
			case static_cast<std::uint8_t>(units::HENRY):						return "henry";
			case static_cast<std::uint8_t>(units::HERTZ):						return "Hz";	//	"hertz";
			case static_cast<std::uint8_t>(units::ACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE):		return "activeEnergyMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::REACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE):	return "reactiveEnergyMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::APPARENT_ENERGY_METER_CONSTANT_OR_PULSE_VALUE):	return "apparentEnergyMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::VOLT_SQUARED_HOURS):			return "voltSquaredHours";
			case static_cast<std::uint8_t>(units::AMPERE_SQUARED_HOURS):		return "ampereSquaredHours";
			case static_cast<std::uint8_t>(units::KILOGRAM_PER_SECOND):			return "kilogramPerSecond";
			case static_cast<std::uint8_t>(units::KELVIN):						return "kelvin";
			case static_cast<std::uint8_t>(units::VOLT_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE):		return "voltSquaredHourMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::AMPERE_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE):	return "ampereSquaredHourMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::METER_CONSTANT_OR_PULSE_VALUE):	return "meterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::PERCENTAGE):					return "percentage";
			case static_cast<std::uint8_t>(units::AMPERE_HOUR):					return "ampereHour";
			case static_cast<std::uint8_t>(units::ENERGY_PER_VOLUME):			return "energyPerVolume";
			case static_cast<std::uint8_t>(units::CALORIFIC_VALUE):				return "calorificValue";
			case static_cast<std::uint8_t>(units::MOLE_PERCENT):				return "molePercent";
			case static_cast<std::uint8_t>(units::MASS_DENSITY):				return "massDensity";
			case static_cast<std::uint8_t>(units::PASCAL_SECOND):				return "pascalSecond";
				 
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_SECOND):		return "m³/s";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_MINUTE):		return "m³/min";
			case static_cast<std::uint8_t>(units::KILOGRAM_PER_HOUR):			return "kg/h";
			case static_cast<std::uint8_t>(units::CUBIC_FEET):					return "cft";
			case static_cast<std::uint8_t>(units::US_GALLON):					return "Impl. gal.";
			case static_cast<std::uint8_t>(units::US_GALLON_PER_MINUTE):		return "Impl. gal./min";
			case static_cast<std::uint8_t>(units::US_GALLON_PER_HOUR):			return "Impl. gal./h";
			case static_cast<std::uint8_t>(units::DEGREE_FAHRENHEIT):			return "°F";
				 
			case static_cast<std::uint8_t>(units::UNIT_RESERVED):				return "reserved";
			case static_cast<std::uint8_t>(units::OTHER_UNIT):					return "other";
			case static_cast<std::uint8_t>(units::COUNT):						return "counter";
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


