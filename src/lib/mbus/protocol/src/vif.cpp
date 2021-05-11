/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/vif.h>

namespace smf {
    namespace mbus {
        vif::vif(char c) : value_(static_cast<std::uint8_t>(c)) {}

        vif::vif(std::uint8_t c) : value_(c) {}

        std::uint8_t vif::get_value() const { return value_ & 0x7f; }

        bool vif::is_extended() const { return (value_ & 0x80) == 0x80; }

        bool vif::has_enhanced_id() const { return value_ == 0x79; }

        bool vif::is_ext_7B() const { return value_ == 0x7B; }

        bool vif::is_ext_7D() const { return value_ == 0x7D; }

        bool vif::is_man_specific() const { return value_ == 0x7F; }

        bool vif::is_date() const { return ((value_ & 0x7E) == 0x6C) && ((value_ & 0x01) == 0); }

        bool vif::is_time() const { return ((value_ & 0x7E) == 0x6C) && ((value_ & 0x01) == 1); }

        std::pair<std::int8_t, unit> vif::get_vib_type() const {

            //
            //	integral values
            //
            if ((value_ & 0x78) == 0x00) {
                //	0x78 = 0b0111 1000
                //	E000 0nnn
                //	Energy (EW01 forward) 1-0:1.8*255
                return {(value_ & 0x07) - 3, unit::WATT_HOUR};
            } else if ((value_ & 0x78) == 0x8) {
                //	0x78 = 0b0111 1000
                //	0x08 = 0b0000 1000
                //	E000 1nnn
                return {(value_ & 0x07), unit::JOULE};
            } else if ((value_ & 0x78) == 0x10) {
                //	b0001 0000 = 0x10
                //	E001 0nnn
                //	Volume a 1 o(nnn-6) m3
                return {static_cast<std::int8_t>(value_ & 0x07) - 6, unit::CUBIC_METRE};
            } else if ((value_ & 0x78) == 0x18) {
                //	b0001 1000 = 0x18
                //	 E001 1nnn (mass)
                return {static_cast<std::int8_t>((value_ & 0x07) - 3), unit::KILOGRAM};
            } else if (value_ == 0x22) {
                //	 0010 0010 (hours in service) 3 bytes
                return {0, unit::HOUR};
            } // else if ((value_ & 0x7C) == 0x20) {
            //	//	b0111 1100 = 0x7C
            //	//	b0010 0000 = 0x20
            //	//	 E010 00nn (on time)
            //}
            // else if ((value_ & 0x7C) == 0x24) {
            //	//	b0010 0100 = 0x24
            //	//	 E010 01nn
            //	//	time
            //}

            //
            //	averaged values
            //
            else if ((value_ & 0x78) == 0x28) {
                //	b0010 1000 = 0x28
                //	 E010 1nnn (Power)
                return {static_cast<std::int8_t>((value_ & 0x07) - 3), unit::WATT};
            } else if ((value_ & 0x78) == 0x30) {
                //	b0011 0000 = 0x30
                //	 E011 0nnn (Power)
                return {static_cast<std::int8_t>((value_ & 0x07)), unit::JOULE_PER_HOUR};
            } else if ((value_ & 0x78) == 0x38) {
                //	b0011 1000 = 0x38
                //	 E011 1nnn (Volume Flow)
                return {static_cast<std::int8_t>((value_ & 0x07) - 6), unit::CUBIC_METRE_PER_HOUR};
            } else if ((value_ & 0x78) == 0x40) {
                //	b0100 0000 = 0x40
                //	 E100 0nnn (Volume Flow ext.)
                return {static_cast<std::int8_t>((value_ & 0x07) - 7), unit::CUBIC_METRE_PER_MINUTE};
            } else if ((value_ & 0x78) == 0x48) {
                //	b0100 1000 = 0x48
                //	 E100 1nnn (Volume Flow ext.)
                return {static_cast<std::int8_t>((value_ & 0x07) - 9), unit::CUBIC_METRE_PER_SECOND};
            } else if ((value_ & 0x78) == 0x50) {
                //	b0101 0000 = 0x50
                //	 E101 0nnn (Mass flow)
                return {static_cast<std::int8_t>((value_ & 0x07) - 3), unit::KILOGRAM_PER_HOUR};
            }

            //
            //	instantaneous values
            //
            else if ((value_ & 0x7C) == 0x58) {
                //	b0101 1000 = 0x58
                //	 E101 10nn (Flow Temperature)
                return {static_cast<std::int8_t>((value_ & 0x03) - 3), unit::DEGREE_CELSIUS};
            } else if ((value_ & 0x7C) == 0x5C) {
                //	b0101 1100 = 0x5C
                //	 E101 11nn (Return Temperature)
                return {static_cast<std::int8_t>((value_ & 0x03) - 3), unit::DEGREE_CELSIUS};
            } else if ((value_ & 0x7C) == 0x40) {
                //	b0100 0000 = 0x40
                //	 E110 00nn (Temperature Difference)
                return {static_cast<std::int8_t>((value_ & 0x03) - 3), unit::KELVIN};
            } else if ((value_ & 0x7C) == 0x34) {
                //	b0110 0100 = 0x34
                //	 E110 01nn (External Temperature)
                return {static_cast<std::int8_t>((value_ & 0x03) - 3), unit::DEGREE_CELSIUS};
            } else if ((value_ & 0x7C) == 0x68) {
                //	b0110 1000 = 0x68
                //	 E110 10nn (Pressure)
                return {static_cast<std::int8_t>((value_ & 0x03) - 3), unit::BAR};
            } else if ((value_ & 0x7E) == 0x6C) {
                //	b0111 1110
                //	b0110 1100 = 0x6C - Date (actual or associated data field =0010b, type G with a storage number / function)
                //	 E110 110n (Time Point)
                return {0, unit::COUNT};
            } else if (value_ == 0x6C) {
                //	E110 1100 Date (actual or associated with a storage number / function)
                //	data field =0010b, type G
            } else if (value_ == 0x6D) {
                //	E110 1101
                //	DT01: Extended date and time Time and date to sec.data field = 0110b, type I point(actual or associated with
                //a storage number / function) 	Meaning depends on data field. 	Time to s 	data field= 0100b, type F 	data field= 0011b,
                //type J 	data field= 0110b, type I
                return {0, unit::COUNT};
            } else if (value_ == 0x6E) {
                //	 E110 1110 (Units for H.C.A.)
                // unit_ = mbus::units::UNIT_RESERVED;
                // return "HCA";
                return {0, unit::COUNT};
            } else if (value_ == 0x6F) {
                //	 E110 1111 (reserved)
                return {0, unit::COUNT};
            } else if (value_ == 0x63) {
                //	 0110 0011 Temperature difference (K)
                return {0, unit::KELVIN};
            }

            //
            //	parameters
            //
            else if ((value_ & 0x7C) == 0x70) {
                //	b0111 0000 = 0x70
                //	 E111 00nn (Averaging Duration)
                return {0, decode_time_unit(value_ & 0x03)};

            } else if ((value_ & 0x7C) == 0x74) {
                //	b0111 0100 = 0x70
                //	 E111 01nn (Actuality Duration)
                //	Time since last radio telegram reception
                //	see Open Metering System Specification Vol.2 – Primary Communication Issue 3.0.1 / 2011-01-29 (Release)
                //	2.2.2.1 Synchronous versus asynchronous transmission
                //	OBIS code: 81 06 00 03 74 FF
                return {0, decode_time_unit(value_ & 0x03)};
            } else if (value_ == 0x78) {
                //	 E111 1000 (Fabrication No)
                return {0, unit::OTHER_UNIT};
            } else if (value_ == 0x79) {
                //	 E111 1001 (Enhanced Identification)
                //	Data = Identification No. (8 digit BCD)
                // length_ = mbus::DFC_8_DIGIT_BCD;
            } else if (value_ == 0x7A) {
                //	 E111 1010 (Bus Address)
                //	For EN 13757-2: one byte link layer address, data type C(x = 8) For EN 13757 - 4: data field 110b(6 byte
                //header - ID) or 111b(full 8 byte header)
            } else if (value_ == 0x7B) {
                //	 E111 1011 Extension of VIF-codes (true VIF is given in the first VIFE)
                // return state::STATE_VIF_EXT_7B;
            } else if (value_ == 0x7C) {
                //	E111 1100 (VIF in following string (length in first byte))
            } else if (value_ == 0x7D) {
                //	E111 1101 Extension of VIF-codes (true VIF is given in the first VIFE)
                //	Multiplicative correction factor for value (not unit)
                // return state::STATE_VIF_EXT_7D;
            } else if (value_ == 0x7F) {
                //	 E111 1111 (Manufacturer Specific)
            } 

            else {
                ;
            }

            return {0, unit::UNDEFINED_};
        }

    } // namespace mbus
} // namespace smf
