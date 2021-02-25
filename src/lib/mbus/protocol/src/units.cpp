/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/units.h>

namespace smf
{
	namespace mbus
	{
		const char* get_unit_name(unit u)
		{
			switch (u)
			{
			case unit::YEAR:						return "year";
			case unit::MONTH:						return "month";
			case unit::WEEK:						return "week";
			case unit::DAY:							return "day";
			case unit::HOUR:						return "hour";
			case unit::MIN:							return "minute";
			case unit::SECOND:						return "second";
			case unit::DEGREE:						return "degree";
			case unit::DEGREE_CELSIUS:				return "Celsius";
			case unit::CURRENCY:					return "currency";
			case unit::METER:						return "meter";
			case unit::METRE_PER_SECOND:			return "meterPerSecond";
			case unit::CUBIC_METRE:					return "cubicMeter";	//	return "cubicMeter";
			case unit::CUBIC_METRE_CORRECTED:		return "cubicMeterCorrected";
			case unit::CUBIC_METRE_PER_HOUR:		return "cubicMeterPerHour";
			case unit::CUBIC_METRE_PER_HOUR_CORRECTED:return "cubicMeterPerHourCorrected";
			case unit::CUBIC_METRE_PER_DAY:			return "cubicMeterPerDay";
			case unit::CUBIC_METRE_PER_DAY_CORRECTED:	return "cubicMeterPerDayCorrected";
			case unit::LITRE:						return "L";	//	litre
			case unit::KILOGRAM:					return "kg";	//	return "kilogram";
			case unit::NEWTON:						return "N";	//	newton
			case unit::NEWTONMETRE:					return "newtonMetre";
			case unit::UNIT_PASCAL:					return "P";	//	pascal
			case unit::BAR:							return "bar";
			case unit::JOULE:						return "J";	//	joule
			case unit::JOULE_PER_HOUR:				return "joulePerHour";
			case unit::WATT:						return "W";	//	watt
			case unit::VOLT_AMPERE:					return "voltAmpere";
			case unit::VAR:							return "var";
			case unit::WATT_HOUR:					return "Wh";	//	"wattHour"
			case unit::VOLT_AMPERE_HOUR:			return "voltAmpereHour";
			case unit::VAR_HOUR:					return "varHour";
			case unit::AMPERE:						return "A";	//	ampere
			case unit::COULOMB:						return "C";	//	coulomb
			case unit::VOLT:						return "V";	//	volt
			case unit::VOLT_PER_METRE:				return "voltPerMetre";
			case unit::FARAD:						return "F";	//	farad
			case unit::OHM:							return "Ohm"; //	ohm (V/A)
			case unit::OHM_METRE:					return "ohmMetre";
			case unit::WEBER:						return "Wb";	//	weber (V s)
			case unit::TESLA:						return "T";	//	Tesla
			case unit::AMPERE_PER_METRE:			return "amperePerMetre";
			case unit::HENRY:						return "henry";
			case unit::HERTZ:						return "Hz";	//	"hertz";
			case unit::ACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE:		return "activeEnergyMeterConstantOrPulseValue";
			case unit::REACTIVE_ENERGY_METER_CONSTANT_OR_PULSE_VALUE:	return "reactiveEnergyMeterConstantOrPulseValue";
			case unit::APPARENT_ENERGY_METER_CONSTANT_OR_PULSE_VALUE:	return "apparentEnergyMeterConstantOrPulseValue";
			case unit::VOLT_SQUARED_HOURS:			return "voltSquaredHours";
			case unit::AMPERE_SQUARED_HOURS:		return "ampereSquaredHours";
			case unit::KILOGRAM_PER_SECOND:			return "kilogramPerSecond";
			case unit::KELVIN:						return "kelvin";
			case unit::VOLT_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE:		return "voltSquaredHourMeterConstantOrPulseValue";
			case unit::AMPERE_SQUARED_HOUR_METER_CONSTANT_OR_PULSE_VALUE:	return "ampereSquaredHourMeterConstantOrPulseValue";
			case unit::METER_CONSTANT_OR_PULSE_VALUE:	return "meterConstantOrPulseValue";
			case unit::PERCENTAGE:					return "percentage";
			case unit::AMPERE_HOUR:					return "ampereHour";
			case unit::ENERGY_PER_VOLUME:			return "energyPerVolume";
			case unit::CALORIFIC_VALUE:				return "calorificValue";
			case unit::MOLE_PERCENT:				return "molePercent";
			case unit::MASS_DENSITY:				return "massDensity";
			case unit::PASCAL_SECOND:				return "pascalSecond";
				 
			case unit::CUBIC_METRE_PER_SECOND:		return "cubicMeter/s";
			case unit::CUBIC_METRE_PER_MINUTE:		return "cubicMeter/min";
			case unit::KILOGRAM_PER_HOUR:			return "kg/h";
			case unit::CUBIC_FEET:					return "cft";
			case unit::US_GALLON:					return "Impl. gal.";
			case unit::US_GALLON_PER_MINUTE:		return "Impl. gal./min";
			case unit::US_GALLON_PER_HOUR:			return "Impl. gal./h";
			case unit::DEGREE_FAHRENHEIT:			return "Fahrenheit";
				 
			case unit::UNIT_RESERVED:				return "reserved";
			case unit::OTHER_UNIT:					return "other";
			case unit::COUNT:						return "counter";
			default:
				break;
			}

			//	not-a-unit
			return "not-a-unit";

		}

	}
}


