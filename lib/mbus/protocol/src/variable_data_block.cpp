/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/variable_data_block.h>
#include <cyng/factory.h>
#include <iostream>
#include <boost/assert.hpp>

namespace node 
{
	variable_data_block::variable_data_block()
	: length_(0)
	, storage_nr_(0)
	, tariff_(0)
	, sub_unit_(0)
	, scaler_(0)
	, unit_(mbus::UNDEFINED_)
	, dt_flag_date_(false)
	, dt_flag_time_(false)
	, value_()
	{}
	
	cyng::object const& variable_data_block::get_value() const
	{
		return value_;
	}

	std::int8_t variable_data_block::get_scaler() const
	{
		return scaler_;
	}

	std::size_t variable_data_block::decode(cyng::buffer_t const& data, std::size_t offset)
	{
		//
		//	0 - instantaneous value
		//	1 - maximum value
		//	2 - minimum value
		//	all other values signal an error
		//
		std::uint8_t func_field = (data.at(offset) & 0x30) >> 4;
		//BOOST_ASSERT_MSG(func_field < 3, "function field out of range");
		if (func_field > 2) {
			std::cerr << "***FATAL: function field out of range: " << +func_field << std::endl;
			return data.size();
		}

		//
		//	length (and data type)
		//
 		length_ = (data.at(offset) & 0x0F);	
 		
 		//
 		//	initial value
 		//
 		storage_nr_= (data.at(offset) & 0x40);
		
		//
		//	finalize DIB
		//
		offset = finalize_dib(data, offset);
		
		//
		//	decode VIB
		//
		
		//
		//	get VIF
		//
		offset = decode_vif(data, offset);
		
		//
		//	get data
		//
		offset = decode_data(data, offset);
		
		return offset;
	}
	
	std::size_t variable_data_block::finalize_dib(cyng::buffer_t const& data, std::size_t offset)
	{
		int pos{ 0 };
		while ((data.at(offset++) & 0x80) == 0x80) {
			sub_unit_ += (((data.at(offset) & 0x40) >> 6) << pos);
			tariff_ += ((data.at(offset) & 0x30) >> 4) << (pos * 2);
			storage_nr_ += ((data.at(offset) & 0x0F) << ((pos * 4) + 1));
			pos++;
		}
		
		return offset;
	}
	
	std::size_t variable_data_block::decode_data(cyng::buffer_t const& data, std::size_t offset)
	{
        switch (length_) {
        case 0x00:
        case 0x08: 
			//	no data - selection for readout request
			value_.clear();
            break;
        case 0x01:
			//	i8
			value_ = cyng::make_object(data.at(offset++));
            break;
        case 0x02:
			//	i16
            if (dt_flag_date_) {
                int day = (0x1f) & data.at(offset);
                int year1 = ((0xe0) & data.at(offset++)) >> 5;
                int month = (0x0f) & data.at(offset);
                int year2 = ((0xf0) & data.at(offset++)) >> 1;
                int year = (2000 + year1 + year2);

//                 Calendar calendar = Calendar.getInstance();
// 
//                 calendar.set(year, month - 1, day, 0, 0, 0);
// 
//                 dataValue = calendar.getTime();
//                 dataValueType = DataValueType.DATE;
            }
            else {
				value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8));

//                 dataValue = Long.valueOf((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8));
//                 dataValueType = DataValueType.LONG;
            }
            break;
        case 0x03:
			//	i24
            if ((data.at(offset + 2) & 0x80) == 0x80) {
                // negative
				value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16) | 0xff << 24);
//                 dataValue = Long.valueOf(
//                         (data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16) | 0xff << 24);
            }
            else {
				value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16));
//                 dataValue = Long
//                         .valueOf((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16));
            }
//             dataValueType = DataValueType.LONG;
            break;
        case 0x04:
			//	i32
            if (dt_flag_time_) {
                int min = (data.at(offset++) & 0x3f);
                int hour = (data.at(offset) & 0x1f);
                int yearh = (0x60 & data.at(offset++)) >> 5;
                int day = (data.at(offset) & 0x1f);
                int year1 = (0xe0 & data.at(offset++)) >> 5;
                int mon = (data.at(offset) & 0x0f);
                int year2 = (0xf0 & data.at(offset++)) >> 1;

                if (yearh == 0) {
                    yearh = 1;
                }

                int year = 1900 + 100 * yearh + year1 + year2;

//                 Calendar calendar = Calendar.getInstance();
// 
//                 calendar.set(year, mon - 1, day, hour, min, 0);
// 
//                 dataValue = calendar.getTime();
//                 dataValueType = DataValueType.DATE;
            }
            else {
				value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8)
                        | ((data.at(offset++) & 0xff) << 16) | ((data.at(offset++) & 0xff) << 24));
//                 dataValue = Long.valueOf((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8)
//                         | ((data.at(offset++) & 0xff) << 16) | ((data.at(offset++) & 0xff) << 24));
//                 dataValueType = DataValueType.LONG;
            }
            break;
        case 0x05:
			//	f32
			offset += 4;
			value_ = cyng::make_object(0.0f);
//             Float doubleDatavalue = ByteBuffer.wrap(buffer, i, 4).order(ByteOrder.LITTLE_ENDIAN).getFloat();
//             i += 4;
//             dataValue = Double.valueOf(doubleDatavalue);
//             dataValueType = DataValueType.DOUBLE;
            break;
        case 0x06:
			//	i48
            if ((data.at(offset + 5) & 0x80) == 0x80) {
                // negative
				value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16)
                                | ((data.at(offset++) & 0xff) << 24) | (((long) data.at(offset++) & 0xff) << 32)
                                | (((long) data.at(offset++) & 0xff) << 40) | (0xffl << 48) | (0xffl << 56));
//                 dataValue = Long
//                         .valueOf((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16)
//                                 | ((data.at(offset++) & 0xff) << 24) | (((long) data.at(offset++) & 0xff) << 32)
//                                 | (((long) data.at(offset++) & 0xff) << 40) | (0xffl << 48) | (0xffl << 56));
            }
            else {
				value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8)
                        | ((data.at(offset++) & 0xff) << 16) | ((data.at(offset++) & 0xff) << 24)
                        | (((long) data.at(offset++) & 0xff) << 32) | (((long) data.at(offset++) & 0xff) << 40));
//                 dataValue = Long.valueOf((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8)
//                         | ((data.at(offset++) & 0xff) << 16) | ((data.at(offset++) & 0xff) << 24)
//                         | (((long) data.at(offset++) & 0xff) << 32) | (((long) data.at(offset++) & 0xff) << 40));
            }
//             dataValueType = DataValueType.LONG;
            break;
        case 0x07:
			//	i64
			value_ = cyng::make_object((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16)
                    | ((data.at(offset++) & 0xff) << 24) | (((long) data.at(offset++) & 0xff) << 32)
                    | (((long) data.at(offset++) & 0xff) << 40) | (((long) data.at(offset++) & 0xff) << 48)
                    | (((long) data.at(offset++) & 0xff) << 56));
//             dataValue = Long.valueOf((data.at(offset++) & 0xff) | ((data.at(offset++) & 0xff) << 8) | ((data.at(offset++) & 0xff) << 16)
//                     | ((data.at(offset++) & 0xff) << 24) | (((long) data.at(offset++) & 0xff) << 32)
//                     | (((long) data.at(offset++) & 0xff) << 40) | (((long) data.at(offset++) & 0xff) << 48)
//                     | (((long) data.at(offset++) & 0xff) << 56));
//             dataValueType = DataValueType.LONG;
            break;
        case 0x09:
            offset = set_BCD(data, offset, 1);
            break;
        case 0x0a:
            offset = set_BCD(data, offset, 2);
            break;
        case 0x0b:
            offset = set_BCD(data, offset, 3);
            break;
        case 0x0c:
            offset = set_BCD(data, offset, 4);
            break;
        case 0x0e:
            offset = set_BCD(data, offset, 6);
            break;
        case 0x0d:
		{
            int variableLength = data.at(offset++) & 0xff;
            int dataLength0x0d;

            if (variableLength < 0xc0) {
                dataLength0x0d = variableLength;
            }
            else if ((variableLength >= 0xc0) && (variableLength <= 0xc9)) {
                dataLength0x0d = 2 * (variableLength - 0xc0);
            }
            else if ((variableLength >= 0xd0) && (variableLength <= 0xd9)) {
                dataLength0x0d = 2 * (variableLength - 0xd0);
            }
            else if ((variableLength >= 0xe0) && (variableLength <= 0xef)) {
                dataLength0x0d = variableLength - 0xe0;
            }
            else if (variableLength == 0xf8) {
                dataLength0x0d = 4;
            }
            else {
				std::cerr << "Unsupported LVAR Field: " << variableLength << std::endl;
				BOOST_ASSERT_MSG(false, "Unsupported LVAR Field");
            }

            // TODO check this:
            // if (variableLength >= 0xc0) {
            // throw new DecodingException("Variable length (LVAR) field >= 0xc0: " + variableLength);
            // }

//             byte[] rawData = new byte[dataLength0x0d];
// 
//             for (int j = 0; j < dataLength0x0d; j++) {
//                 rawData[j] = buffer[i + dataLength0x0d - 1 - j];
//             }
            offset += dataLength0x0d;

//             dataValue = new String(rawData);
//             dataValueType = DataValueType.STRING;
		}
            break;
        default:
			std::cerr << "***FATAL: Unknown Data Field in DIF: " << +length_ << std::endl;
			//BOOST_ASSERT_MSG(false, "Unknown Data Field in DIF");
			break;
        }
		
		return offset;
	}
	
    std::size_t variable_data_block::set_BCD(cyng::buffer_t const& data, std::size_t offset, std::size_t range) 
	{
		value_ = cyng::make_object(cyng::buffer_t(data.data() + offset, data.data() + offset + range));
        return offset + range;
    }
	
	std::size_t variable_data_block::decode_vif(cyng::buffer_t const& data, std::size_t offset)
	{
		const std::uint8_t vif = data.at(offset++) & 0xFF;
		bool more_vifs = false;
		
        if (vif == 0xfb) {
//             decodeAlternateExtendedVif(data.at(offset));
			BOOST_ASSERT_MSG(false, "decodeAlternateExtendedVif(not implemented)");
            if ((data.at(offset) & 0x80) == 0x80) {
                more_vifs = true;
            }
            ++offset;
        }
        else if ((vif & 0x7f) == 0x7c) {
//             offset += decodeUserDefinedVif(buffer, offset);
			BOOST_ASSERT_MSG(false, "decodeUserDefinedVif(not implemented)");
			if ((vif & 0x80) == 0x80) {
                more_vifs = true;
            }
        }
        else if (vif == 0xfd) {
			auto name = decode_main_ext_vif(vif);
            if ((data.at(offset) & 0x80) == 0x80) {
                more_vifs = true;
            }
            ++offset;
        }
        else {
            auto descr = decode_main_vif(vif);
            if ((vif & 0x80) == 0x80) {
                more_vifs = true;
            }
#ifdef _DEBUG
			std::cout << descr << " ";
#endif
		}

        if (more_vifs) {
            while ((data.at(offset++) & 0x80) == 0x80) {
                // ToDO: implement
            }
        }
		
		return offset;
	}
	
	std::string variable_data_block::decode_main_vif(std::uint8_t vif)
	{
        return ((vif & 0x40) == 0)
			? read_e0(vif)
			: read_e1(vif)
			;
	}
	
	std::string variable_data_block::read_e1(std::uint8_t vif)
	{
        // E1
		return ((vif & 0x20) == 0)
			? read_e10(vif)
			: read_e11(vif)
			;
    }

	std::string variable_data_block::read_e11(std::uint8_t vif)
	{
        // E11
		return ((vif & 0x10) == 0)
			? read_e110(vif)
			: read_e111(vif)
			;
    }

	std::string variable_data_block::read_e111(std::uint8_t vif)
	{
        // E111
        if ((vif & 0x08) == 0) {
            // E111 0
			decodeTimeUnit(vif);
			return ((vif & 0x04) == 0)
				? "AVERAGING_DURATION"
				: "ACTUALITY_DURATION"
				;
        }
        else {
            // E111 1
            if ((vif & 0x04) == 0) {
                // E111 10
                if ((vif & 0x02) == 0) {
                    // E111 100
					return ((vif & 0x01) == 0)
						? "FABRICATION_NO"	// E111 1000
						: "EXTENDED_IDENTIFICATION"	// E111 1001
						;
                }
                else {
                    // E111 101
                    if ((vif & 0x01) == 0) {
						return "ADDRESS";
                    }
                    else {
                        // E111 1011
                        // Codes used with extension indicator 0xFB (table 29 of DIN EN 13757-3:2011)
						std::cerr << "Trying to decode a mainVIF even though it is an alternate extended vif" << std::endl;
						BOOST_ASSERT_MSG(false, "Trying to decode a mainVIF even though it is an alternate extended vif");
                    }
                }
            }
            else {
                // E111 11
                if ((vif & 0x02) == 0) {
                    // E111 110
                    if ((vif & 0x01) == 0) {
                        // E111 1100
                        // Extension indicator 0xFC: VIF is given in following string
						return "NOT_SUPPORTED";
                    }
                    else {
                        // E111 1101
                        // Extension indicator 0xFD: main VIFE-code extension table (table 28 of DIN EN
                        // 13757-3:2011)
						std::cerr << "Trying to decode a mainVIF even though it is a main extended vif" << std::endl;
						BOOST_ASSERT_MSG(false, "Trying to decode a mainVIF even though it is a main extended vif");
                    }
                }
                else {
                    // E111 111
					return ((vif & 0x01) == 0)
						? "FUTURE_VALUE"	// E111 1110
						: "MANUFACTURER_SPECIFIC"	// E111 1111
						;
                }
            }
        }
		return "!";
    }

	std::string variable_data_block::read_e110(std::uint8_t vif)
	{
        // E110
        if ((vif & 0x08) == 0) {
            // E110 0
            if ((vif & 0x04) == 0) {
                // E110 00
				scaler_ = (vif & 0x03) - 3;
				unit_ = mbus::KELVIN;
				return "TEMPERATURE_DIFFERENCE";
            }
            else {
                // E110 01
                scaler_ = (vif & 0x03) - 3;
                unit_ = mbus::DEGREE_CELSIUS;
				return "EXTERNAL_TEMPERATURE";
			}
        }
        else {
            // E110 1
            if ((vif & 0x04) == 0) {
                // E110 10
                scaler_ = (vif & 0x03) - 3;
                unit_ = mbus::BAR;
				return "PRESSURE";
			}
            else {
                // E110 11
                if ((vif & 0x02) == 0) {
                    // E110 110
                    if ((vif & 0x01) == 0) {
                        // E110 1100
                        dt_flag_date_ = true;
						return "DATE";
					}
                    else {
                        // E110 1101
                        dt_flag_time_ = true;
						return "DATE_TIME";
					}
                }
                else {
                    // E110 111
                    if ((vif & 0x01) == 0) {
                        // E110 1110
                        unit_ = mbus::UNIT_RESERVED;
						return "HCA";
					}
                    else {
						return "NOT_SUPPORTED";
                    }
                }

            }
        }
		return "!";
	}

	std::string variable_data_block::read_e10(std::uint8_t vif)
	{
        // E10
        if ((vif & 0x10) == 0) {
            // E100
            if ((vif & 0x08) == 0) {
                // E100 0
                scaler_ = (vif & 0x07) - 7;
                 unit_ = mbus::CUBIC_METRE_PER_MINUTE;
				return "VOLUME_FLOW_EXT";
			}
            else {
                // E100 1
                scaler_ = (vif & 0x07) - 9;
				unit_ = mbus::CUBIC_METRE_PER_SECOND;
				return "VOLUME_FLOW_EXT";
			}
        }
        else {
            // E101
            if ((vif & 0x08) == 0) {
                // E101 0
                scaler_ = (vif & 0x07) - 3;
                unit_ = mbus::KILOGRAM_PER_HOUR;
				return "MASS_FLOW";
			}
            else {
                // E101 1
                if ((vif & 0x04) == 0) {
                    // E101 10
                    scaler_ = (vif & 0x03) - 3;
                    unit_ = mbus::DEGREE_CELSIUS;
					return "FLOW_TEMPERATURE";
				}
                else {
                    // E101 11
                    scaler_ = (vif & 0x03) - 3;
                    unit_ = mbus::DEGREE_CELSIUS;
					return "RETURN_TEMPERATURE";
				}
            }
        }
		return "!";
	}

	std::string variable_data_block::read_e0(std::uint8_t vif)
	{
        // E0
		return ((vif & 0x20) == 0)
			? read_e00(vif)
			: read_01(vif)
			;
    }

	std::string variable_data_block::read_01(std::uint8_t vif)
	{
        // E01
        if ((vif & 0x10) == 0) {
            // E010
            if ((vif & 0x08) == 0) {
                // E010 0
				decodeTimeUnit(vif);
				return ((vif & 0x04) == 0)
					? "ON_TIME"	// E010 00
					: "OPERATING_TIME"	// E010 01
					;
            }
            else {
                // E010 1
                scaler_ = (vif & 0x07) - 3;
                unit_ = mbus::WATT;
				return "POWER";
			}
        }
        else {
            // E011
            if ((vif & 0x08) == 0) {
                // E011 0
                scaler_ = vif & 0x07;
                unit_ = mbus::JOULE_PER_HOUR;
				return "POWER";
			}
            else {
                // E011 1
                scaler_ = (vif & 0x07) - 6;
                unit_ = mbus::CUBIC_METRE_PER_HOUR;
				return "VOLUME_FLOW";
			}
        }
		return "!";
	}

	std::string variable_data_block::read_e00(std::uint8_t vif)
	{
        // E00
        if ((vif & 0x10) == 0) {
            // E000
            if ((vif & 0x08) == 0) {
                // E000 0
                scaler_ = (vif & 0x07) - 3;
                unit_ = mbus::WATT_HOUR;
				return "ENERGY";
			}
            else {
                // E000 1
                scaler_ = vif & 0x07;
                unit_ = mbus::JOULE;
				return "ENERGY";
			}
        }
        else {
            // E001
            if ((vif & 0x08) == 0) {
                // E001 0
                scaler_ = (vif & 0x07) - 6;
                unit_ = mbus::CUBIC_METRE;
				return "VOLUME";
			}
            else {
                // E001 1
                scaler_ = (vif & 0x07) - 3;
                unit_ = mbus::KILOGRAM;
				return "MASS";
			}
        }
		return "!";
	}

    void variable_data_block::decodeTimeUnit(std::uint8_t vif) 
	{
        if ((vif & 0x02) == 0) {
            if ((vif & 0x01) == 0) {
                unit_ = mbus::SECOND;
            }
            else {
                unit_ = mbus::MIN;
            }
        }
        else {
            if ((vif & 0x01) == 0) {
                unit_ = mbus::HOUR;
            }
            else {
                unit_ = mbus::DAY;
            }
        }
    }
    
    // implements table 28 of DIN EN 13757-3:2013
	std::string variable_data_block::decode_main_ext_vif(std::uint8_t vif)
    {
		switch (vif & 0x7f) {

		case 0x0b:  // E000 1011
			return "PARAMETER_SET_ID";
		case 0x0c: // E000 1100
			return "MODEL_VERSION";
		case 0x0d: // E000 1101
			return "HARDWARE_VERSION";
		case 0x0e: // E000 1110
			return "FIRMWARE_VERSION";
		case 0x0f: // E000 1111
			return "OTHER_SOFTWARE_VERSION";
		case 0x10: // E001 0000
			return "CUSTOMER_LOCATION";
		case 0x11: // E001 0001
			return "CUSTOMER";
		case 0x12: // E001 0010
			return "ACCSESS_CODE_USER";
		case 0x13: // E001 0011
			return "ACCSESS_CODE_OPERATOR";
		case 0x14: // E001 0100
			return "ACCSESS_CODE_SYSTEM_OPERATOR";
		case 0x15: // E001 0101
			return "ACCSESS_CODE_SYSTEM_DEVELOPER";
		case 0x16: // E001 0110
			return "PASSWORD";
		case 0x17: // E001 0111
			return "ERROR_FLAGS";
		case 0x18: // E001 1000
			return "ERROR_MASK";
		case 0x19: // E001 1001
			return "SECURITY_KEY";
		case 0x1a: // E001 1010
			return "DIGITAL_OUTPUT";
		case 0x1b: // E001 1011
			return "DIGITAL_INPUT";
		case 0x1c: // E001 1100
			return "BAUDRATE";
		case 0x1d: // E001 1101
			return "RESPONSE_DELAY_TIME";
		case 0x1e: // E001 1110
			return "RETRY";
		case 0x1f: // E001 1111
			return "REMOTE_CONTROL";
		case 0x20: // E010 0000
			return "FIRST_STORAGE_NUMBER_CYCLIC";
		case 0x21: // E010 0001
			return "LAST_STORAGE_NUMBER_CYCLIC";
		case 0x22: // E010 0010
			return "SIZE_STORAGE_BLOCK";
		case 0x23: // E010 0011
			return "RESERVED";
		case 0x24: // E010 01nn
			//this.unit = unitFor(vif);
			return "STORAGE_INTERVALL";
		case 0x28: // E010 1000
			unit_ = mbus::MONTH;
			return "STORAGE_INTERVALL";
		case 0x29: // E010 1001
			unit_ = mbus::YEAR;
			return "STORAGE_INTERVALL";
		case 0x2a: // E010 1010
			return "OPERATOR_SPECIFIC_DATA";
		case 0x2b: // E010 1011
			unit_ = mbus::SECOND;
			return "TIME_POINT";
		case 0x2c:// E010 11nn
				  //             this.unit = unitFor(vif);
			return "DURATION_LAST_READOUT";
		case 0x30: // E011 00nn
			switch (vif & 0x03) {
			case 0: // E011 0000
				return "NOT_SUPPORTED"; // TODO: TARIF_START (Date/Time)
			default:
				//                 this.unit = unitFor(vif);
				break;
			}
			return "TARIF_DURATION";
		case 0x34:	// E011 01nn
					//             this.unit = unitFor(vif);
			return "TARIF_PERIOD";
		case 0x38: // E011 1000
			unit_ = mbus::MONTH;
			return "TARIF_PERIOD";
		case 0x39: // E011 1001
			unit_ = mbus::YEAR;
			return "TARIF_PERIOD";
		case 0x40:	// E100 0000
			scaler_ = (vif & 0x0f) - 9;
			unit_ = mbus::VOLT;
			return "VOLTAGE";
		case 0x50: // E101 0000
			scaler_ = (vif & 0x0f) - 12;
			unit_ = mbus::AMPERE;
			return "CURRENT";
		case 0x60: // E110 0000
			return "RESET_COUNTER";
		case 0x61: // E110 0001
			return "CUMULATION_COUNTER";
		case 0x62: // E110 0010
			return "CONTROL_SIGNAL";
		case 0x63: // E110 0011
			return "DAY_OF_WEEK"; // 1 = Monday; 7 = Sunday; 0 = all Days
		case 0x64: // E110 0100
			return "WEEK_NUMBER";
		case 0x65: // E110 0101
			return "TIME_POINT_DAY_CHANGE";
		case 0x66: // E110 0110
			return "PARAMETER_ACTIVATION_STATE";
		case 0x67: // E110 0111
			return "SPECIAL_SUPPLIER_INFORMATION";
		case 0x68:	// E110 10nn
					//             this.unit = unitBiggerFor(vif);
			return "LAST_CUMULATION_DURATION";
		case 0x6c:	// E110 11nn
					//             this.unit = unitBiggerFor(vif);
			return "OPERATING_TIME_BATTERY";
		case 0x70: // E111 0000
			return "NOT_SUPPORTED"; // TODO: BATTERY_CHANGE_DATE_TIME
		case 0x71: // E111 0001
			return "NOT_SUPPORTED"; // TODO: RF_LEVEL dBm
		case 0x72: // E111 0010
			return "NOT_SUPPORTED"; // TODO: DAYLIGHT_SAVING (begin, ending, deviation)
		case 0x73: // E111 0011
			return "NOT_SUPPORTED"; // TODO: Listening window management data type L
		case 0x74: // E111 0100
			unit_ = mbus::DAY;
			return "REMAINING_BATTERY_LIFE_TIME";
		case 0x75: // E111 0101
			return "NUMBER_STOPS";
		case 0x76: // E111 0110
			return "MANUFACTURER_SPECIFIC";
		default:
			break;
		}

		return ((vif & 0x7f) >= 0x77)	// E111 0111 - E111 1111
			? "RESERVED"
			: "NOT_SUPPORTED"
			;

    }
	
}	//	node

