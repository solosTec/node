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
			case static_cast<std::uint8_t>(units::DEGREE_CELSIUS):				return "Celsius";
			case static_cast<std::uint8_t>(units::CURRENCY):					return "currency";
			case static_cast<std::uint8_t>(units::METER):						return "meter";
			case static_cast<std::uint8_t>(units::METRE_PER_SECOND):			return "meterPerSecond";
			case static_cast<std::uint8_t>(units::CUBIC_METRE):					return "cubicMeter";	//	return "cubicMeter";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_CORRECTED):		return "cubicMeterCorrected";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_HOUR):		return "cubicMeterPerHour";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_HOUR_CORRECTED):return "cubicMeterPerHourCorrected";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_DAY):			return "cubicMeterPerDay";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_DAY_CORRECTED):	return "cubicMeterPerDayCorrected";
			case static_cast<std::uint8_t>(units::LITRE):						return "L";	//	litre
			case static_cast<std::uint8_t>(units::KILOGRAM):					return "kg";	//	return "kilogram";
			case static_cast<std::uint8_t>(units::NEWTON):						return "N";	//	newton
			case static_cast<std::uint8_t>(units::NEWTONMETRE):					return "newtonMetre";
			case static_cast<std::uint8_t>(units::UNIT_PASCAL):					return "P";	//	pascal
			case static_cast<std::uint8_t>(units::BAR):							return "bar";
			case static_cast<std::uint8_t>(units::JOULE):						return "J";	//	joule
			case static_cast<std::uint8_t>(units::JOULE_PER_HOUR):				return "joulePerHour";
			case static_cast<std::uint8_t>(units::WATT):						return "W";	//	watt
			case static_cast<std::uint8_t>(units::VOLT_AMPERE):					return "voltAmpere";
			case static_cast<std::uint8_t>(units::VAR):							return "var";
			case static_cast<std::uint8_t>(units::WATT_HOUR):					return "Wh";	//	"wattHour"
			case static_cast<std::uint8_t>(units::VOLT_AMPERE_HOUR):			return "voltAmpereHour";
			case static_cast<std::uint8_t>(units::VAR_HOUR):					return "varHour";
			case static_cast<std::uint8_t>(units::AMPERE):						return "A";	//	ampere
			case static_cast<std::uint8_t>(units::COULOMB):						return "C";	//	coulomb
			case static_cast<std::uint8_t>(units::VOLT):						return "V";	//	volt
			case static_cast<std::uint8_t>(units::VOLT_PER_METRE):				return "voltPerMetre";
			case static_cast<std::uint8_t>(units::FARAD):						return "F";	//	farad
			case static_cast<std::uint8_t>(units::OHM):							return "Ohm"; //	ohm (V/A)
			case static_cast<std::uint8_t>(units::OHM_METRE):					return "ohmMetre";
			case static_cast<std::uint8_t>(units::WEBER):						return "Wb";	//	weber (V s)
			case static_cast<std::uint8_t>(units::TESLA):						return "T";	//	Tesla
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
				 
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_SECOND):		return "cubicMeter/s";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_MINUTE):		return "cubicMeter/min";
			case static_cast<std::uint8_t>(units::KILOGRAM_PER_HOUR):			return "kg/h";
			case static_cast<std::uint8_t>(units::CUBIC_FEET):					return "cft";
			case static_cast<std::uint8_t>(units::US_GALLON):					return "Impl. gal.";
			case static_cast<std::uint8_t>(units::US_GALLON_PER_MINUTE):		return "Impl. gal./min";
			case static_cast<std::uint8_t>(units::US_GALLON_PER_HOUR):			return "Impl. gal./h";
			case static_cast<std::uint8_t>(units::DEGREE_FAHRENHEIT):			return "Fahrenheit";
				 
			case static_cast<std::uint8_t>(units::UNIT_RESERVED):				return "reserved";
			case static_cast<std::uint8_t>(units::OTHER_UNIT):					return "other";
			case static_cast<std::uint8_t>(units::COUNT):						return "counter";
			default:
				break;
			}

			//	not-a-unit
			return "not-a-unit";

		}

		std::u8string get_unit_name_u8(std::uint8_t u)
		{
			switch (u)
			{
			case static_cast<std::uint8_t>(units::YEAR):						return u8"year";
			case static_cast<std::uint8_t>(units::MONTH):						return u8"month";
			case static_cast<std::uint8_t>(units::WEEK):						return u8"week";
			case static_cast<std::uint8_t>(units::DAY):							return u8"day";
			case static_cast<std::uint8_t>(units::HOUR):						return u8"hour";
			case static_cast<std::uint8_t>(units::MIN):							return u8"minute";
			case static_cast<std::uint8_t>(units::SECOND):						return u8"second";
			case static_cast<std::uint8_t>(units::DEGREE):						return u8"degree";
			case static_cast<std::uint8_t>(units::DEGREE_CELSIUS):				return u8"°C";
			case static_cast<std::uint8_t>(units::CURRENCY):					return u8"currency";
			case static_cast<std::uint8_t>(units::METER):						return u8"meter";
			case static_cast<std::uint8_t>(units::METRE_PER_SECOND):			return u8"meterPerSecond";
			case static_cast<std::uint8_t>(units::CUBIC_METRE):					return u8"m³";	//	return "cubicMeter";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_CORRECTED):		return u8"cubicMeterCorrected";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_HOUR):		return u8"cubicMeterPerHour";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_HOUR_CORRECTED):return u8"cubicMeterPerHourCorrected";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_DAY):			return u8"cubicMeterPerDay";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_DAY_CORRECTED):	return u8"cubicMeterPerDayCorrected";
			case static_cast<std::uint8_t>(units::LITRE):						return u8"litre";
			case static_cast<std::uint8_t>(units::KILOGRAM):					return u8"kg";	//	return "kilogram";
			case static_cast<std::uint8_t>(units::NEWTON):						return u8"N";	//	newton
			case static_cast<std::uint8_t>(units::NEWTONMETRE):					return u8"newtonMetre";
			case static_cast<std::uint8_t>(units::UNIT_PASCAL):					return u8"pascal";
			case static_cast<std::uint8_t>(units::BAR):							return u8"bar";
			case static_cast<std::uint8_t>(units::JOULE):						return u8"J";	//	joule
			case static_cast<std::uint8_t>(units::JOULE_PER_HOUR):				return u8"joulePerHour";
			case static_cast<std::uint8_t>(units::WATT):						return u8"W";	//	watt
			case static_cast<std::uint8_t>(units::VOLT_AMPERE):					return u8"voltAmpere";
			case static_cast<std::uint8_t>(units::VAR):							return u8"var";
			case static_cast<std::uint8_t>(units::WATT_HOUR):					return u8"Wh";	//	"wattHour"
			case static_cast<std::uint8_t>(units::VOLT_AMPERE_HOUR):			return u8"voltAmpereHour";
			case static_cast<std::uint8_t>(units::VAR_HOUR):					return u8"varHour";
			case static_cast<std::uint8_t>(units::AMPERE):						return u8"ampere";
			case static_cast<std::uint8_t>(units::COULOMB):						return u8"coulomb";
			case static_cast<std::uint8_t>(units::VOLT):						return u8"V";	//	volt
			case static_cast<std::uint8_t>(units::VOLT_PER_METRE):				return u8"voltPerMetre";
			case static_cast<std::uint8_t>(units::FARAD):						return u8"F";	//	farad
			case static_cast<std::uint8_t>(units::OHM):							return u8"Ω"; //	ohm (V/A)
			case static_cast<std::uint8_t>(units::OHM_METRE):					return u8"ohmMetre";
			case static_cast<std::uint8_t>(units::WEBER):						return u8"Wb";	//	weber (V s)
			case static_cast<std::uint8_t>(units::TESLA):						return u8"tesla";
			case static_cast<std::uint8_t>(units::AMPERE_PER_METRE):			return u8"amperePerMetre";
			case static_cast<std::uint8_t>(units::HENRY):						return u8"henry";
			case static_cast<std::uint8_t>(units::HERTZ):						return u8"Hz";	//	"hertz";
			case static_cast<std::uint8_t>(units::ACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE):		return u8"activeEnergyMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::REACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE):	return u8"reactiveEnergyMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::APPARENT_ENERGY_METER_CONSTANT_OR_PULSE_VALUE):	return u8"apparentEnergyMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::VOLT_SQUARED_HOURS):			return u8"voltSquaredHours";
			case static_cast<std::uint8_t>(units::AMPERE_SQUARED_HOURS):		return u8"ampereSquaredHours";
			case static_cast<std::uint8_t>(units::KILOGRAM_PER_SECOND):			return u8"kilogramPerSecond";
			case static_cast<std::uint8_t>(units::KELVIN):						return u8"kelvin";
			case static_cast<std::uint8_t>(units::VOLT_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE):		return u8"voltSquaredHourMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::AMPERE_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE):	return u8"ampereSquaredHourMeterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::METER_CONSTANT_OR_PULSE_VALUE):	return u8"meterConstantOrPulseValue";
			case static_cast<std::uint8_t>(units::PERCENTAGE):					return u8"percentage";
			case static_cast<std::uint8_t>(units::AMPERE_HOUR):					return u8"ampereHour";
			case static_cast<std::uint8_t>(units::ENERGY_PER_VOLUME):			return u8"energyPerVolume";
			case static_cast<std::uint8_t>(units::CALORIFIC_VALUE):				return u8"calorificValue";
			case static_cast<std::uint8_t>(units::MOLE_PERCENT):				return u8"molePercent";
			case static_cast<std::uint8_t>(units::MASS_DENSITY):				return u8"massDensity";
			case static_cast<std::uint8_t>(units::PASCAL_SECOND):				return u8"pascalSecond";
				 
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_SECOND):		return u8"m³/s";
			case static_cast<std::uint8_t>(units::CUBIC_METRE_PER_MINUTE):		return u8"m³/min";
			case static_cast<std::uint8_t>(units::KILOGRAM_PER_HOUR):			return u8"kg/h";
			case static_cast<std::uint8_t>(units::CUBIC_FEET):					return u8"cft";
			case static_cast<std::uint8_t>(units::US_GALLON):					return u8"Impl. gal.";
			case static_cast<std::uint8_t>(units::US_GALLON_PER_MINUTE):		return u8"Impl. gal./min";
			case static_cast<std::uint8_t>(units::US_GALLON_PER_HOUR):			return u8"Impl. gal./h";
			case static_cast<std::uint8_t>(units::DEGREE_FAHRENHEIT):			return u8"°F";
				 
			case static_cast<std::uint8_t>(units::UNIT_RESERVED):				return u8"reserved";
			case static_cast<std::uint8_t>(units::OTHER_UNIT):					return u8"other";
			case static_cast<std::uint8_t>(units::COUNT):						return u8"counter";
			default:
				break;
			}

			//	not-a-unit
			return u8"not-a-unit";
		}
		
		const char* get_unit_name(units u) {
			return get_unit_name(static_cast<std::uint8_t>(u));
		}

	}
}


