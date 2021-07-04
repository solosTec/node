/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/dif.h>
#include <smf/mbus/vif.h>

namespace smf {
    namespace mbus {
        vif::vif(char c)
            : value_(1, static_cast<std::uint8_t>(c)) {}

        vif::vif(std::uint8_t c)
            : value_(1, c) {}

        std::uint8_t vif::get_value() const {
            BOOST_ASSERT(!value_.empty());
            return value_.at(0);
        }

        std::uint8_t vif::get_ext_value(std::size_t idx) const {
            if (idx + 1 < value_.size()) {
                return value_.at(idx + 1);
            }
            return 0;
        }

        std::uint8_t vif::get_and(std::uint8_t v) const { return get_value() & v; }

        bool vif::is_extended() const { return (get_value() & 0x80) == 0x80; }

        std::uint8_t vif::get_last() const {
            BOOST_ASSERT(!value_.empty());
            return value_.back();
        }

        bool vif::is_complete() const { return !((get_last() & 0x80) == 0x80); }

        std::size_t vif::add(char c) {
            value_.push_back(c);
            return value_.size();
        }

        std::size_t vif::get_vife_size() const {
            BOOST_ASSERT(!value_.empty());
            return value_.size() - 1;
        }

        bool vif::is_manufacturer_specific(std::size_t idx) const {
            return (idx < value_.size()) ? (value_.at(idx) == 0xFF) : false;
        }

        bool vif::is_date() const { return (get_and(0x7E) == 0x6C) && (get_and(0x01) == 0); }
        bool vif::is_time() const { return (get_and(0x7E) == 0x6C) && (get_and(0x01) == 1); }

        // bool vif::is_equal(std::uint8_t value) const { return value_ == value; }

        std::tuple<std::int8_t, unit, cyng::obis> vif::get_vib_type(std::uint8_t medium, dif const &d) const {

            //
            //	integral values
            //  see OMS-Spec_Vol2_Primary_v200.pdf from 2009-07-20
            //
            if ((get_value() & 0x78) == 0x00) {
                //	0x78 = 0b0111 1000
                //	->E000 0nnn
                //	Energy (EW01 forward) 1-0:1.8*255
                if (medium == 5) {
                    //  Energy (A), total, current value
                    return {(get_value() & 0x07) - 3, unit::WATT_HOUR, cyng::make_obis(medium, 0, 1, 0, 0, 255)};
                }
                //  Active energy import (+A), current value
                return {(get_value() & 0x07) - 3, unit::WATT_HOUR, cyng::make_obis(medium, 0, 1, 8, 0, 255)};
            } else if ((get_value() & 0x78) == 0x8) {
                //	0x78 = 0b0111 1000
                //	0x08 = 0b0000 1000
                //	->E000 1nnn
                return {(get_value() & 0x07), unit::JOULE, cyng::make_obis(medium, 0, 1, 0, 0, 255)};
            } else if ((get_value() & 0x78) == 0x10) {
                //	b0001 0000 = 0x10
                //	->E001 0nnn
                //	Volume a 1 o(nnn-6) m3
                //  category: VM01 (not extended)
                //  INDICATOR - 3 == single value Betriebsvolumen Ausspeisung
                //  MODE - 1 == temperaturkompensiert
                return {
                    static_cast<std::int8_t>(get_value() & 0x07) - 6, unit::CUBIC_METRE, cyng::make_obis(medium, 0, 3, 1, 0, 255)};
            } else if ((get_value() & 0x78) == 0x18) {
                //	b0001 1000 = 0x18
                //	->E001 1nnn (mass)
                return {
                    static_cast<std::int8_t>((get_value() & 0x07) - 3), unit::KILOGRAM, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if (get_value() == 0x22) {
                //	 0010 0010 (hours in service) 3 bytes
                return {0, unit::HOUR, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } // else if ((get_value() & 0x7C) == 0x20) {
            //	//	b0111 1100 = 0x7C
            //	//	b0010 0000 = 0x20
            //	//	 E010 00nn (on time)
            //}
            // else if ((get_value() & 0x7C) == 0x24) {
            //	//	b0010 0100 = 0x24
            //	//	 E010 01nn
            //	//	time
            //}

            //
            //	averaged values
            //
            else if ((get_value() & 0x78) == 0x28) {
                //	b0010 1000 = 0x28
                //	->E010 1nnn (Power)
                return {static_cast<std::int8_t>((get_value() & 0x07) - 3), unit::WATT, cyng::make_obis(medium, 0, 8, 0, 0, 255)};
            } else if ((get_value() & 0x78) == 0x30) {
                //	b0011 0000 = 0x30
                //	->E011 0nnn (Power)
                return {
                    static_cast<std::int8_t>((get_value() & 0x07)), unit::JOULE_PER_HOUR, cyng::make_obis(medium, 0, 8, 0, 0, 255)};
            } else if ((get_value() & 0x78) == 0x38) {
                //	b0011 1000 = 0x38
                //	->E011 1nnn (Volume Flow)
                return {
                    static_cast<std::int8_t>((get_value() & 0x07) - 6),
                    unit::CUBIC_METRE_PER_HOUR,
                    cyng::make_obis(medium, 0, 9, 0, 0, 255)};
            } else if ((get_value() & 0x78) == 0x40) {
                //	b0100 0000 = 0x40
                //	->E100 0nnn (Volume Flow ext.)
                return {
                    static_cast<std::int8_t>((get_value() & 0x07) - 7),
                    unit::CUBIC_METRE_PER_MINUTE,
                    cyng::make_obis(medium, 0, 8, 0, 0, 255)};
            } else if ((get_value() & 0x78) == 0x48) {
                //	b0100 1000 = 0x48
                //	->E100 1nnn (Volume Flow ext.)
                return {
                    static_cast<std::int8_t>((get_value() & 0x07) - 9),
                    unit::CUBIC_METRE_PER_SECOND,
                    cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if ((get_value() & 0x78) == 0x50) {
                //	b0101 0000 = 0x50
                //	->E101 0nnn (Mass flow)
                return {
                    static_cast<std::int8_t>((get_value() & 0x07) - 3),
                    unit::KILOGRAM_PER_HOUR,
                    cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            }

            //
            //	instantaneous values
            //
            else if ((get_value() & 0x7C) == 0x58) {
                //	b0101 1000 = 0x58
                //	->E101 10nn (Flow Temperature)
                return {
                    static_cast<std::int8_t>((get_value() & 0x03) - 3),
                    unit::DEGREE_CELSIUS,
                    cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if ((get_value() & 0x7C) == 0x5C) {
                //	b0101 1100 = 0x5C
                //	->E101 11nn (Return Temperature)
                return {
                    static_cast<std::int8_t>((get_value() & 0x03) - 3),
                    unit::DEGREE_CELSIUS,
                    cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if ((get_value() & 0x7C) == 0x40) {
                //	b0100 0000 = 0x40
                //	->E110 00nn (Temperature Difference)
                return {static_cast<std::int8_t>((get_value() & 0x03) - 3), unit::KELVIN, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if ((get_value() & 0x7C) == 0x34) {
                //	b0110 0100 = 0x34
                //	 E110 01nn (External Temperature)
                return {
                    static_cast<std::int8_t>((get_value() & 0x03) - 3),
                    unit::DEGREE_CELSIUS,
                    cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if ((get_value() & 0x7C) == 0x68) {
                //	b0110 1000 = 0x68
                //	 E110 10nn (Pressure)
                return {static_cast<std::int8_t>((get_value() & 0x03) - 3), unit::BAR, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if ((get_value() & 0x7E) == 0x6C) {
                //	b0111 1110
                //	b0110 1100 = 0x6C - Date (actual or associated data field =0010b, type G with a storage number / function)
                //	 E110 110n (Time Point)
                return {0, unit::COUNT, cyng::make_obis(medium, 0, 0, 9, 0, 255)};
            } else if (get_value() == 0x6C) {
                //	E110 1100 Date (actual or associated with a storage number / function)
                //	data field =0010b, type G
                //  Current date at time of transmission
                return {0, unit::COUNT, cyng::make_obis(medium, 0, 0, 9, 2, 255)};
            } else if (get_value() == 0x6D) {
                //	E110 1101
                //	DT01: Extended date and time Time and date to sec.data field = 0110b, type I point(actual or associated with
                // a storage number / function) 	Meaning depends on data field. 	Time to s 	data field= 0100b, type F
                // data field= 0011b, type J 	data field= 0110b, type I
                //  Current time at time of transmission
                return {0, unit::COUNT, cyng::make_obis(medium, 0, 0, 9, 1, 255)};
            } else if (get_value() == 0x6E) {
                //	 E110 1110 (Units for H.C.A.)
                // unit_ = mbus::units::UNIT_RESERVED;
                // return "HCA";
                return {0, unit::COUNT, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if (get_value() == 0x6F) {
                //	 E110 1111 (reserved)
                return {0, unit::COUNT, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if (get_value() == 0x63) {
                //	 0110 0011 Temperature difference (K)
                return {0, unit::KELVIN, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            }

            //
            //	parameters
            //
            else if ((get_value() & 0x7C) == 0x70) {
                //	b0111 0000 = 0x70
                //	 E111 00nn (Averaging Duration)
                return {0, decode_time_unit(get_value() & 0x03), cyng::make_obis(medium, 0, 0, 0, 0, 255)};

            } else if ((get_value() & 0x7C) == 0x74) {
                //	b0111 0100 = 0x70
                //	 E111 01nn (Actuality Duration)
                //	Time since last radio telegram reception
                //	see Open Metering System Specification Vol.2 – Primary Communication Issue 3.0.1 / 2011-01-29 (Release)
                //	2.2.2.1 Synchronous versus asynchronous transmission
                //	OBIS code: 81 06 00 03 74 FF
                return {0, decode_time_unit(get_value() & 0x03), cyng::make_obis(1, 0, 0, 8, 1, 255)};
            } else if (get_value() == 0x78) {
                //	 E111 1000 (Fabrication No)
                return {0, unit::OTHER_UNIT, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
            } else if (get_value() == 0x79) {
                //	 E111 1001 (Enhanced Identification)
                //	Data = Identification No. (8 digit BCD)
                // length_ = mbus::DFC_8_DIGIT_BCD;
                //  C.90.1 - IEC address
                return {0, unit::OTHER_UNIT, cyng::make_obis(0, 0, 96, 1, 0, 255)};
            } else if (get_value() == 0x7A) {
                //	 E111 1010 (Bus Address)
                //	For EN 13757-2: one byte link layer address, data type C(x = 8) For EN 13757 - 4: data field 110b(6 byte
                // header - ID) or 111b(full 8 byte header)
            } else if (get_value() == 0x7B) {
                //	 E111 1011 Extension of VIF-codes (true VIF is given in the first VIFE)
                // return state::STATE_VIF_EXT_7B;
            } else if (get_value() == 0x7C) {
                //	E111 1100 (VIF in following string (length in first byte))
            } else if (get_value() == 0x7D) {
                //	E111 1101 Extension of VIF-codes (true VIF is given in the first VIFE)
                //	Multiplicative correction factor for value (not unit)
                // return state::STATE_VIF_EXT_7D;
            } else if (get_value() == 0x7F) {
                //	 E111 1111 (Manufacturer Specific)
                //  serial number
                return {0, unit::COUNT, cyng::make_obis(0, 0, 96, 1, 0, 255)};
            }

            else {
                ;
            }

            return {0, unit::UNDEFINED_, cyng::make_obis(medium, 0, 0, 0, 0, 255)};
        }

        std::tuple<vib_category, cyng::obis, std::int8_t, unit, bool>
        get_vib_category(std::uint8_t medium, std::uint8_t tariff, vif const &v) {

            if (v.get_value() == 0xfd) {
                //  CA Current [A]
                //  CL Control
                //  CT Compliance Tes
                //  DP Duration/Period (Period of nominal transmission)
                //  EJ Energy [GJ]
                //  EW Energy [kWh]
                //  FR Frequency [Hz]
                //  ID Identification Number (Ownership number/Metering Point ID/Unique message idendtification)
                //  MM Meter Management
                //  VV Voltage [V]
                //  YD Descriptor
                if (v.get_ext_value(0) == 0x74) {
                    //  remaining capacity (C.6.1)
                    return {vib_category::MM09, cyng::make_obis(0, 0, 96, 6, 1, 255), 0, unit::DAY, true};
                }
                BOOST_ASSERT_MSG(false, "ToDo");
                return {vib_category::UNDEF, cyng::make_obis(medium, 0, 0, 0, tariff, 255), 0, unit::UNDEFINED_, false};
            } else if (v.get_and(0x78) == 0x00) {
                //  EW Energy [kWh] <10e-6 ... 10e+1> forward
                //	0xF8 = 0b1111 1000
                //	->E000 0nnn
                if (v.is_extended()) {
                    //  ToDo:
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {
                    vib_category::EW01,
                    cyng::make_obis(medium, 0, 1, 8, tariff, 255),
                    (v.get_and(0x07)) - 3,
                    unit::WATT_HOUR,
                    true};
            } else if (v.get_and(0x78) == 0x8) {
                //	0x78 = 0b0111 1000
                //	0x08 = 0b0000 1000
                //	->E000 1nnn
                if (v.is_extended()) {
                    //  ToDo: EW04, EW07
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {vib_category::EJ01, cyng::make_obis(medium, 0, 1, 0, tariff, 255), v.get_and(0x07), unit::JOULE, true};
            } else if (v.get_and(0x78) == 0x10) {
                //  VM Volume [m³]
                //	b0001 0000 = 0x10
                //	->E001 0nnn
                if (v.is_extended()) {
                    //  ToDo: VF02, VF03
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {
                    vib_category::VM01,
                    cyng::make_obis(medium, 0, 2, 0, tariff, 255),
                    v.get_and(0x07) - 6,
                    unit::CUBIC_METRE,
                    true};
            } else if (v.get_and(0x78) == 0x18) {
                //	b0001 1000 = 0x18
                //	->E001 1nnn (mass)
                //  This is not defined by OMS-Spec Vol2 AnnexB D438
                return {
                    vib_category::UNDEF, cyng::make_obis(medium, 0, 0, 0, tariff, 255), v.get_and(0x07) - 3, unit::KILOGRAM, true};
            } else if (v.get_and(0x78) == 0x28) {
                //	    b0010 1000 = 0x28
                //  PW01 0010 1nnn
                if (v.is_extended()) {
                    //  PW03, ... PW10
                    switch (v.get_vife_size()) {
                    case 1:
                        //  1010 1nnn 0011 1100
                        return {
                            vib_category::PW03,
                            cyng::make_obis(medium, 0, 1, 7, tariff, 255),
                            v.get_and(0x07) - 3,
                            unit::WATT,
                            true};
                    case 2:
                        //  PW07: 1010 1nnn - 1111 1100 - 0001 0000
                        //  PW09: 1010 1nnn - 1111 1100 - 0000 1100
                        if (v.is_manufacturer_specific(1)) {
                            //  1010 1nnn - 1111 1111 - 0000 0001
                            switch (v.get_ext_value(1)) {
                            case 1:
                                return {
                                    vib_category::PW07,
                                    cyng::make_obis(medium, 0, 21, 7, tariff, 255),
                                    v.get_and(0x07) - 6,
                                    unit::WATT,
                                    true};
                            case 2:
                                return {
                                    vib_category::PW07,
                                    cyng::make_obis(medium, 0, 41, 7, tariff, 255),
                                    v.get_and(0x07) - 6,
                                    unit::WATT,
                                    true};
                            case 3:
                                return {
                                    vib_category::PW07,
                                    cyng::make_obis(medium, 0, 61, 7, tariff, 255),
                                    v.get_and(0x07) - 6,
                                    unit::WATT,
                                    true};
                            default:
                                return {
                                    vib_category::PW07,
                                    cyng::make_obis(medium, 0, 1, 7, tariff, v.get_ext_value(1)),
                                    v.get_and(0x07) - 6,
                                    unit::WATT,
                                    true};
                            }
                        }
                        return {
                            vib_category::PW07,
                            cyng::make_obis(medium, 0, 1, 7, tariff, 255),
                            v.get_and(0x07) - 6,
                            unit::WATT,
                            true};
                    case 3:
                    default:
                        break;
                    }
                }
                return {vib_category::PW01, cyng::make_obis(medium, 0, 1, 7, tariff, 255), v.get_and(0x07) - 3, unit::WATT, true};

            } else if (v.get_and(0xF8) == 0x30) {
                //  PJ Power [kJ/h]
                //	    b0011 0000 = 0x30
                //	PJ01 0011 0nnn
                return {
                    vib_category::PJ01,
                    cyng::make_obis(medium, 0, 8, 0, tariff, 255),
                    v.get_and(0x07) - 3,
                    unit::JOULE_PER_HOUR,
                    true};

            } else if (v.get_and(0x78) == 0x38) {
                //  VF Volume Flow [m³/h]
                if (v.is_extended()) {
                    //  ToDo: VF02, VF03
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {
                    vib_category::VF01,
                    cyng::make_obis(medium, 0, 9, 0, tariff, 255),
                    v.get_and(0x07) - 6,
                    unit::CUBIC_METRE_PER_HOUR,
                    true};

            } else if (v.get_and(0x78) == 0x40) {
                //  This is not defined by OMS-Spec Vol2 AnnexB D438
                //	b0100 0000 = 0x40
                //	->E100 0nnn (Volume Flow ext.)
                return {
                    vib_category::UNDEF,
                    cyng::make_obis(medium, 0, 0, 0, tariff, 255),
                    v.get_and(0x07) - 7,
                    unit::CUBIC_METRE_PER_MINUTE,
                    true};

            } else if (v.get_and(0x78) == 0x48) {
                //  This is not defined by OMS-Spec Vol2 AnnexB D438
                //	b0100 1000 = 0x48
                //	->E100 1nnn (Volume Flow ext.)
                return {
                    vib_category::UNDEF,
                    cyng::make_obis(medium, 0, 0, 0, tariff, 255),
                    v.get_and(0x07) - 9,
                    unit::CUBIC_METRE_PER_SECOND,
                    true};

            } else if (v.get_and(0xFC) == 0x74) {
                //  DP Duration/Period
                //	0111 01nn
                return {vib_category::DP01, cyng::make_obis(medium, 0, 0, 1, 2, 255), 0, decode_time_unit(v.get_and(0x03)), true};

            } else if (v.get_and(0xFC) == 0x70) {
                //  DP Duration/
                //  0111 00nn
                return {vib_category::DP02, cyng::make_obis(medium, 0, 0, 8, 0, 255), 0, decode_time_unit(v.get_and(0x03)), true};

            } else if (v.get_and(0x7C) == 0x6C) {
                //  DT Date / Time (Duration and Time stamp)
                //  E110 1100
                if (v.is_extended()) {
                    //  ToDo: DT04
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {vib_category::DT02, cyng::make_obis(medium, 0, 0, 9, 2, 255), 0, unit::COUNT, true};

            } else if (v.get_and(0x7C) == 0x6D) {
                //  DT Date / Time (Duration and Time stamp)
                //  E110 1101
                if (v.is_extended()) {
                    //  ToDo: DT03
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {vib_category::DT01, cyng::make_obis(medium, 0, 0, 0, tariff, 255), 0, unit::COUNT, true};

            } else if (v.get_and(0xFC) == 0x6E) {
                //  HC Heat Cost Allocation unit
                if (v.is_extended()) {
                    //  ToDo: HC02
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {vib_category::HC01, cyng::make_obis(medium, 0, 1, 0, tariff, 255), 0, unit::COUNT, true};

            } else if (v.get_value() == 0x22) {
                //	 0010 0010 (hours in service) 3 bytes
                //  This is not defined by OMS-Spec Vol2 AnnexB D438
                return {vib_category::UNDEF, cyng::make_obis(medium, 0, 0, 0, tariff, 255), 0, unit::HOUR, true};
            } else if (v.get_value() == 0x78) {
                //  0111 1000 (Fabrication No)
                return {vib_category::ID01, cyng::make_obis(medium, 0, 96, 1, 0, 255), 0, unit::OTHER_UNIT, true};

            } else if (v.get_value() == 0x79) {
                //  0111 1001 - (Enhanced) identification = Fabriknummer
                return {vib_category::ID02, cyng::make_obis(0, 0, 96, 1, 255, 255), 0, unit::OTHER_UNIT, true};
            } else if (v.get_value() == 0x7A) {
                //  0111 1010 (Primary address)
                return {vib_category::ID03, cyng::make_obis(medium, 0, 0, 0, tariff, 255), 0, unit::OTHER_UNIT, true};
            } else if (v.get_value() == 0xFB) {
                BOOST_ASSERT_MSG(false, "ToDo");
                //  PD Phase in Degree
                //  PW Power [W]
                //  RH Relative Humidity [%
                //  RP Reactive Power [kvar]
            } else if (v.get_and(0x7C) == 0x5B) {
                //  TC Temperature [°C]
                if (v.is_extended()) {
                    //  ToDo: TC02, TC03
                    BOOST_ASSERT_MSG(false, "ToDo");
                }
                return {
                    vib_category::TC01,
                    cyng::make_obis(medium, 0, 10, 0, tariff, 255),
                    v.get_and(0x03) - 3,
                    unit::DEGREE_CELSIUS,
                    true};
            } else if (v.get_value() == 0x5C) {
                //  TC Temperature [°C]
                //  0101 11nn (Return Temperature)
                return {
                    vib_category::TC02,
                    cyng::make_obis(medium, 0, 11, 0, tariff, 255),
                    v.get_and(0x03) - 3,
                    unit::DEGREE_CELSIUS,
                    true};
            }
            return {vib_category::UNDEF, cyng::make_obis(medium, 0, 0, 0, tariff, 255), 0, unit::UNDEFINED_, false};
        }

        std::string to_string(vib_category vb) {
            switch (vb) {
            case vib_category::CA01:
                return "A 10e-12 ... 10e+3 Curr_L1";
            case vib_category::CA02:
                return "A 10e-12 ... 10e+3 Curr_L2";
            case vib_category::CA03:
                return "A 10e-12 ... 10e+3 Curr_L3";
            case vib_category::CA04:
                return "A 10e-12 ... 10e+3 Curr_N";

            // case vib_category::CL:   // Control
            case vib_category::CL01:
                return "disconnector control state";

            // case vib_category::DP:   // Duration/Period
            case vib_category::DP01:
                return "s, min, h, d actuality dur.";
            case vib_category::DP02:
                return "s, min, h, d average dur";
            case vib_category::DP03:
                return "s, min Period of nominal transmission";
            case vib_category::DP05:
                return "seconds, minutes, hours, days Operation time (Duration of accumulation)";
            case vib_category::DP06:
                return "seconds, minutes, hours, days On time (Duration since power up)";

            // DT,   //	 Date / Time (Duration and Time stamp)
            case vib_category::DT01:
                return "Date+Time / Time forward";
            case vib_category::DT02:
                return "Date forward";
            case vib_category::DT03:
                return "Date+Time / Time backward";
            case vib_category::DT04:
                return "Date backward";

            // EJ,   //	 Energy [GJ]
            case vib_category::EJ01:
                return "GJ 10e-9 ... 10e-2 forward";
            case vib_category::EJ02:
                return "GJ 10e-1 ... 10e0 forward";
            case vib_category::EJ03:
                return "GJ 10e+2 ... 10e+3 forward";
            case vib_category::EJ04:
                return "GJ 10e-9 ... 10e-2 backward";
            case vib_category::EJ05:
                return "GJ 10e-1 ... 10e+0 backward";
            case vib_category::EJ06:
                return "GJ 10e+2 ... 10e+3 backward";

                // EW,   //	 Energy [kWh]
            case vib_category::EW01:
                return "EW01: kWh 10e-6 ... 10e+1 forward";
            case vib_category::EW02:
                return "EW02: kWh 10e+2 ... 10e+3 forward";
            case vib_category::EW03:
                return "EW03: kWh 10e+5 ... 10e+6 forward ";
            case vib_category::EW04:
                return "EW04: kWh 10e-6 ... 10e+1 backward";
            case vib_category::EW05:
                return "EW05: kWh 10e+2 ... 10e+3 backward";
            case vib_category::EW06:
                return "EW06: kWh 10e+5 ... 10e+6 backward";
            case vib_category::EW07:
                return "EW07: kWh 10e-6 ... 10e+1 abs.";
            case vib_category::EW08:
                return "EW08: kWh 10e+2 ... 10e+3 abs.";
            case vib_category::EW09:
                return "EW09: kWh 10e+5 ... 10e+6 abs.";

            // FR, //	 Frequency [Hz]
            case vib_category::FR01:
                return "Hz 10e-3 ... 10e0";

                // HC,   //	 Heat Cost Allocation unit
            case vib_category::HC01:
                return "HCA 10e+0 signed";
            case vib_category::HC02:
                return "HCA 10e+0 unsigned";

            // ID,   //	 Identification Numbers
            case vib_category::ID01:
                return "Fabrication number";
            case vib_category::ID02:
                return "(Enhanced) identification";
            case vib_category::ID03:
                return "Primary address";
            case vib_category::ID04:
                return "Ownership number";
            case vib_category::ID05:
                return "Metering Point ID";
            case vib_category::ID06:
                return "Unique message identification";

            // MM,   //	Meter Management
            case vib_category::MM09:
                return "Remaining battery lifetime";
            // PD,   //	Phase in Degree
            // PJ,   //	Power[kJ / h]
            case vib_category::PJ01:
                return "";

                // PR, //	Pressure[bar]

                // PW,   //	Power[W]
            case vib_category::PW01:
                return "W 10e-3 ... 10e+4 forward";
            case vib_category::PW03:
                return "W 10e-3 ... 10e+4 backward";
            case vib_category::PW04:
                return "W 10e-3 ... 10e+4 cum. forward";
            case vib_category::PW06:
                return "W 10e-3 ... 10e+4 cum. backward";
            case vib_category::PW07:
                return "kW 10e-6 ... 10e+1 abs";
            case vib_category::PW08:
                return "kW 10e+2 ... 10e+3 abs";
            case vib_category::PW09:
                return "kW 10e-6 ... 10e+1 delta";
            case vib_category::PW10:
                return "kW 10e+2 ... 10e+3 delta";

                // RE, //	Reactive Energy[kvarh]
                // RH, //	Relative Humidity[%]
                // RP, //	Reactive Power[kvar]

                // TC:   //	Temperature[°C]
            case vib_category::TC01:
                return "";
            case vib_category::TC02:
                return "";
            case vib_category::TC03:
                return "";

            // VF:   //	Volume Flow[m³ / h]
            case vib_category::VF01:
                return "";
            case vib_category::VF02:
                return "";
            case vib_category::VF03:
                return "";

            // VM: //	Volume[m³]
            case vib_category::VM01:
                return "";
            case vib_category::VM02:
                return "";
                //  VM10, ...

                // VV,     //	Voltage[V]
                // YD, //	Descriptor
            default:
                break;
            }
            return "unknown vib";
        }

    } // namespace mbus
} // namespace smf
