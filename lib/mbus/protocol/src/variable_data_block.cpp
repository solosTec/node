/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/variable_data_block.h>
#include <smf/mbus/dif.h>
#include <smf/mbus/vif.h>
#include <smf/mbus/bcd.h>

#include <cyng/factory.h>
#include <cyng/io/serializer.h>

#include <iostream>
#include <boost/assert.hpp>

namespace node
{

	vdb_reader::vdb_reader()
		: state_(STATE_DIF_)
		, length_(0)
		, function_field_(0)
		, storage_nr_(0)
		, tariff_(0)
		, sub_unit_(0)
		, scaler_(0)
		, unit_(mbus::UNDEFINED_)
		, date_flag_(false)
		, date_time_flag_(false)
		, value_()
	{}

	cyng::object vdb_reader::get_value() const
	{
		return value_;
	}

	std::int8_t vdb_reader::get_scaler() const
	{
		return scaler_;
	}

	mbus::unit_code	vdb_reader::get_unit() const
	{
		return unit_;
	}

	std::uint64_t vdb_reader::get_storage_nr() const
	{
		return storage_nr_;
	}

	std::uint32_t vdb_reader::get_tariff() const
	{
		return tariff_;
	}

	std::uint16_t vdb_reader::get_sub_unit() const
	{
		return sub_unit_;
	}

	std::size_t vdb_reader::decode(cyng::buffer_t const& data, std::size_t offset)
	{
		if (offset < data.size()) {

			//
			//	reset parser state
			//
			state_ = STATE_DIF_;
			date_flag_ = false;
			date_time_flag_ = false;
			scaler_ = 0;

			for( auto pos = data.begin() + offset; pos < data.end(); ++pos) {

				//
				//	update offset
				//
				++offset;

				switch (state_) {
				case STATE_DIF_:
					state_ = state_dif(*pos);
					break;
				case STATE_DIF_EXT_:
					state_ = state_dif_ext(*pos);
					break;
				case STATE_VIF_:
					state_ = state_vif(*pos);
					break;
				case STATE_VIF_EXT_:
					state_ = state_vif_ext(*pos);
					break;
				case STATE_VIF_EXT_7B:
					state_ = state_vife_7b(*pos);
					break;
				case STATE_VIF_EXT_7D:
					state_ = state_vife_7d(*pos);
					break;
				case STATE_DATA_:
					state_ = state_data(*pos);
					break;
				case STATE_DATA_ASCII_:
					state_ = state_data_ascii(*pos);
					break;
					//STATE_DATA_BCD_POSITIVE_,	//	2 bytes
				//STATE_DATA_BCD_NEGATIVE_,	//	2 bytes
				//STATE_DATA_BINARY_,
				case STATE_DATA_7D_08:
					state_ = state_data_7d_08(*pos);
					break;
				case STATE_DATA_7D_17:
					state_ = state_data_7d_17(*pos);
					break;

				default:
					break;
				}

				if (STATE_COMPLETE == state_) {
#ifdef _DEBUG
					std::cout
						<< mbus::get_unit_name(unit_)
						<< ", "
						<< cyng::io::to_str(value_)
						<< ", scaler: "
						<< +scaler_
						<< std::endl
						;
#endif
					return offset;
				}
			}
		}

		return offset;
	}

	vdb_reader::state vdb_reader::state_dif(char c)
	{
		mbus::udif d;
		d.raw_ = c;

		//
		//	see data_field_code
		//
		length_ = d.dif_.code_;

		//
		//	see function_field_code
		//
		//	0 - instantaneous value
		//	1 - maximum value
		//	2 - minimum value
		//	all other values signal an error
		//
		//
		function_field_ = d.dif_.ff_;
		BOOST_ASSERT(function_field_ != 3);

		//
		//	storage number
		//
		storage_nr_ = d.dif_.sn_;

		return (d.dif_.ext_ == 1)
			? STATE_DIF_EXT_
			: STATE_VIF_
			;
	}

	vdb_reader::state vdb_reader::state_dif_ext(char c)
	{
		mbus::udife de;
		de.raw_ = c;

		//
		//	add values
		//
		sub_unit_ += de.dife_.du_;
		tariff_ += de.dife_.tariff_;
		storage_nr_ += de.dife_.sn_;

		BOOST_ASSERT(storage_nr_ <= 0x20000000000);
		BOOST_ASSERT(tariff_ <= 0x100000000);
		BOOST_ASSERT(sub_unit_ <= 0x10000);

		return (de.dife_.ext_ == 1)
			? STATE_DIF_EXT_
			: STATE_VIF_
			;
	}

	vdb_reader::state vdb_reader::state_vif(char c)
	{
		mbus::uvif v;
		v.raw_ = c;

		std::uint8_t const val = v.vif_.val_;

		//
		//	ready for new data
		//
		buffer_.clear();

		//
		//	integral values
		//
		if ((val & 0x78) == 0x00) {
			//	0x78 = 0b0111 1000
			//	E000 0nnn
			//	Energy
			scaler_ = (val & 0x07) - 3;
			unit_ = mbus::WATT_HOUR;

		}
		else if ((val & 0x78) == 0x8) {
			//	0x78 = 0b0111 1000
			//	0x08 = 0b0000 1000
			//	E000 1nnn
			scaler_ = (val & 0x07);
			unit_ = mbus::JOULE;
		}
		else if ((val & 0x78) == 0x10) {
			//	b0001 0000 = 0x10
			//	E001 0nnn
			//	Volume a 1 o(nnn-6) m3 
			scaler_ = static_cast<std::int8_t>(val & 0x07) - 6;
			unit_ = mbus::CUBIC_METRE;
		}
		else if ((val & 0x78) == 0x18) {
			//	b0001 1000 = 0x18
			//	 E001 1nnn (mass)
			scaler_ = (val & 0x07) - 3;
			unit_ = mbus::KILOGRAM;
		}
		else if ((val & 0x7C) == 0x20) {
			//	b0111 1100 = 0x7C
			//	b0010 0000 = 0x20
			//	 E010 00nn (on time)
			decode_time_unit(val & 0x03);
		}
		else if ((val & 0x7C) == 0x24) {
			//	b0010 0100 = 0x24
			//	 E010 01nn
			decode_time_unit(val & 0x03);
		}

		//
		//	averaged values
		//
		else if ((val & 0x78) == 0x28) {
			//	b0010 1000 = 0x28
			//	 E010 1nnn (Power)
			scaler_ = (val & 0x07) - 3;
			unit_ = mbus::WATT;
		}
		else if ((val & 0x78) == 0x30) {
			//	b0011 0000 = 0x30 
			//	 E011 0nnn (Power)
			scaler_ = (val & 0x07);
			unit_ = mbus::JOULE_PER_HOUR;
		}
		else if ((val & 0x78) == 0x38) {
			//	b0011 1000 = 0x38
			//	 E011 1nnn (Volume Flow)
			scaler_ = (val & 0x07) - 6;
			unit_ = mbus::CUBIC_METRE_PER_HOUR;
		}
		else if ((val & 0x78) == 0x40) {
			//	b0100 0000 = 0x40
			//	 E100 0nnn (Volume Flow ext.)
			scaler_ = (val & 0x07) - 7;
			unit_ = mbus::CUBIC_METRE_PER_MINUTE;
		}
		else if ((val & 0x78) == 0x48) {
			//	b0100 1000 = 0x48
			//	 E100 1nnn (Volume Flow ext.)
			scaler_ = (val & 0x07) - 9;
			unit_ = mbus::CUBIC_METRE_PER_SECOND;
		}
		else if ((val & 0x78) == 0x50) {
			//	b0101 0000 = 0x50
			//	 E101 0nnn (Mass flow)
			scaler_ = (val & 0x07) - 3;
			unit_ = mbus::KILOGRAM_PER_HOUR;
		}

		//
		//	instantaneous values
		//
		else if ((val & 0x7C) == 0x58) {
			//	b0101 1000 = 0x58
			//	 E101 10nn (Flow Temperature)
			scaler_ = (val & 0x03) - 3;
			unit_ = mbus::DEGREE_CELSIUS;
		}
		else if ((val & 0x7C) == 0x5C) {
			//	b0101 1100 = 0x5C
			//	 E101 11nn (Return Temperature)
			scaler_ = (val & 0x03) - 3;
			unit_ = mbus::DEGREE_CELSIUS;
		}
		else if ((val & 0x7C) == 0x40) {
			//	b0100 0000 = 0x40
			//	 E110 00nn (Temperature Difference)
			scaler_ = (val & 0x03) - 3;
			unit_ = mbus::KELVIN;
		}
		else if ((val & 0x7C) == 0x34) {
			//	b0110 0100 = 0x34
			//	 E110 01nn (External Temperature)
			scaler_ = (val & 0x03) - 3;
			unit_ = mbus::DEGREE_CELSIUS;
		}
		else if ((val & 0x7C) == 0x68) {
			//	b0110 1000 = 0x68
			//	 E110 10nn (Pressure)
			scaler_ = (val & 0x03) - 3;
			unit_ = mbus::BAR;
		}
		else if ((val & 0x7E) == 0x6C) {
			//	b0111 1110
			//	b0110 1100 = 0x6C - Date (actual or associated data field =0010b, type G with a storage number / function)
			//	 E110 110n (Time Point)
			date_flag_ = ((val & 0x01) == 0);
			date_time_flag_ = ((val & 0x01) == 1);
			unit_ = mbus::COUNT;
		}
		else if (val == 0x6E) {
			//	 E110 1110 (Units for H.C.A.)
			unit_ = mbus::UNIT_RESERVED;
			//return "HCA";
		}
		else if (val == 0x6F) {
			//	 E110 1111 (reserved)
		}

		//
		//	parameters
		//
		else if ((val & 0x7C) == 0x70) {
			//	b0111 0000 = 0x70
			//	 E111 00nn (Averaging Duration)
			decode_time_unit(val & 0x03);
		}
		else if ((val & 0x7C) == 0x74) {
			//	b0111 0100 = 0x70
			//	 E111 01nn (Actuality Duration)
			//	Time since last radio telegram reception
			decode_time_unit(val & 0x03);
		}
		else if (val == 0x78) {
			//	 E111 1000 (Fabrication No)
		}
		else if (val == 0x79) {
			//	 E111 1001 (Enhanced Identification)
		}
		else if (val == 0x7A) {
			//	 E111 1010 (Bus Address)
		}
		else if (val == 0x7B) {
			//	 E111 1011 Extension of VIF-codes (true VIF is given in the first VIFE)
			return STATE_VIF_EXT_7B;
		}
		else if (val == 0x7C) {
			//	E111 1100 (VIF in following string (length in first byte))
		}
		else if (val == 0x7D) {
			//	E111 1101 Extension of VIF-codes (true VIF is given in the first VIFE)
			//	Multiplicative correction factor for value (not unit)
			return STATE_VIF_EXT_7D;
		}
		else if (val == 0x7F) {
			//	 E111 1111 (Manufacturer Specific)
		}

		else {
			;
		}


		return (v.vif_.ext_ == 1)
			? STATE_VIF_EXT_
			: STATE_DATA_
			;
	}

	vdb_reader::state vdb_reader::state_vif_ext(char c)
	{
		mbus::uvif v;
		v.raw_ = c;

		std::uint8_t const val = v.vif_.val_;

		//E00x xxxx	Reserved for object actions (master to slave)
		//or for error codes (slave to master)	0...31
		//E010 0000	per second	32
		//E010 0001	per minute	33
		//E010 0010	per hour	34
		//E010 0011	per day	35
		//E010 0100	per week	36
		//E010 0101	per month	37
		//E010 0110	per year	38
		//E010 0111	per revolution / measurement	39
		//E010 100p	increment per input pulse on input channel #p	40,41
		//E010 101p	increment per output pulse on output channel #p	42,43
		//E010 1100	per liter	44
		//E010 1101	per m³	45
		//E010 1110	per kg	46
		//E010 1111	per K (Kelvin)	47
		//E011 0000	per kWh	48
		//E011 0001	per GJ	49
		//E011 0010	per kW	50
		//E011 0011	per (K*l) (Kelvin*liter)	51
		//E011 0100	per V (Volt)	52
		//E011 0101	per A (Ampere)	53
		//E011 0110	multiplied by sek	54
		//E011 0111	multiplied by sek/V	55
		//E011 1000	multiplied by sek/A	56
		//E011 1001	start date(/time) of 	57
		if (val == 0x32) {
			//E011 1010	VIF contains uncorrected unit instead of corrected unit	58
		}
		//E011 1011	Accumulation only if positive contributions	59
		//E011 1100	Accumulation of abs value only if negative contributions	60
		//E011 1101
		//...
		//E011 1111	Reserved	61...63
		//E100 0000	Lower limit value	64
		//E100 0001	# of exceeds of lower limit	65
		//E100 0010	Date (/time) of begin of first lower limit exceed	66
		//E100 0011	Date (/time) of end of first lower limit exceed	67
		//E100 010x	Reserved	68,69
		//E100 0110	Date (/time) of begin of last lower limit exceed	70
		//E100 0111	Date (/time) of end of last lower limit exceed	71
		//E100 1000	Upper limit value	72
		//E100 1001	# of exceeds of upper limit	73
		//E100 1010	Date (/time) of begin of first upper limit exceed	74
		//E100 1011	Date (/time) of end of first upper limit exceed	75
		//E101 110x	Reserved	76,77
		//E100 1110	Date (/time) of begin of last upper limit exceed	78
		//E100 1111	Date (/time) of end of last upper limit exceed	79
		//E101 00nn	Duration of first lower limit exceed [sec...days]	80...83
		//E101 01nn	Duration of last lower limit exceed [sec...days]	84...87
		//E101 10nn	Duration of first upper limit exceed [sec...days]	88...91
		//E101 11nn	Duration of last upper limit exceed [sec...days]	92...95
		//E110 00nn	Duration of first  [sec...days]	96...99
		//E110 01nn	Duration of last  [sec...days]	100...103
		//E110 100x	Reserved	104...105
		//E110 1010	Date (/time) of begin of first	106
		//E110 1011	Date (/time) of end of first 	107
		//E110 110x	Reserved	108,109
		//E110 1110	Date (/time) of begin of last 	110
		//E110 1111	Date (/time) of end of last 	111
		//E111 0nnn	Multiplicative correction factor: 10nnn-6	112...119
		//E111 10nn	Additive correction constant: 10nn-3 unit of VIF (offset)	120...123
		//E111 1100	Reserved	124
		//E111 1101	Multiplicative correction factor: 1000	125
		//E111 1110	future value	126
		//E111 1111	next VIFE's and data of this block are maufacturer specific	127

		return (v.vif_.ext_ == 1)
			? STATE_VIF_EXT_
			: STATE_DATA_
			;
	}

	vdb_reader::state vdb_reader::state_vife_7b(char c)
	{
		//E000 000n	Energy	10n-1 MWh	0.1MWh to 1MWh
		//E000 001n	Reserved	 	 
		//E000 01nn	Reserved	 	 
		//E000 100n	Energy	10n-1 GJ	0.1GJ to 1GJ
		//E000 101n	Reserved	 	 
		//E000 11nn	Reserved	 	 
		//E001 000n	Volume	10n+2 m³	100m³ to 1000m³
		//E001 001n	Reserved	 	 
		//E001 01nn	Reserved	 	 
		//E001 100n	Mass	10n+2 t	100t to 1000t
		//E001 1010
		//....
		//E010 0000	Reserved	 	 
		//E010 0001	Volume	0.1 feet³	 
		//E010 0010	Volume	0.1 american gallon	 
		//E010 0011	Volume	1 american gallon	 
		//E010 0100	Volume flow	0.001 american gallon/min	 
		//E010 0101	Volume flow	1 american gallon/min	 
		//E010 0110	Volume flow	1 american gallon/h	 
		//E010 0111	Reserved	 	 
		//E010 100n	Power	10n-1 MW	0.1MW to 1MW
		//E010 101n	Reserved	 	 
		//E010 11nn	Reserved	 	 
		//E011 000n	Power	10n-1 GJ/h	0.1GJ/h to 1GJ/h
		//E011 0010
		//....
		//E101 0111	Reserved	 	 
		//E101 10nn	Flow Temperature	10nn-3 °F	0.001°F to 1°F
		//E101 11nn	Return Temperature	10nn-3 °F	0.001°F to 1°F
		//E110 00nn	Temperature Difference	10nn-3 °F	0.001°F to 1°F
		//E110 01nn	External Temperature	10nn-3 °F	0.001°F to 1°F
		//E110 1nnn	Reserved	 	 
		//E111 00nn	Cold / Warm Temperature Limit	10nn-3 °F	0.001°F to 1°F
		//E111 01nn	Cold / Warm Temperature Limit	10nn-3 °C	0.001°C to 1°C
		//E111 1nnn	cumul. count max power	10nnn-3 W	0.001W to 10000W

		return STATE_DATA_;
	}

	vdb_reader::state vdb_reader::state_vife_7d(char c)
	{
		if ((c & 0x7C) == 0x00) {
			//E000 00nn	Credit of 10nn-3 of the nominal local legal currency units
		}
		else if ((c & 0x7C) == 0x04) {
			//E000 01nn	Debit of 10nn-3 of the nominal local legal currency units
		}
		else if ((c & 0x7F) == 0x08) {
			//	E000 1000	Access Number (transmission count)
			//	2 data bytes
			BOOST_ASSERT(length_ == 2);
			return STATE_DATA_7D_08;
		}
		//Enhanced Identification
		//E000 1001	Medium (as in fixed header)
		//E000 1010	Manufacturer (as in fixed header)
		//E000 1011	Parameter set identification
		//E000 1100	Model / Version
		//E000 1101	Hardware version #
		//E000 1110	Firmware version #
		//E000 1111	Software version #
		//Implementation of all TC294 WG1 requirements
		//(improved selection ..)
		//E001 0000	Customer location
		//E001 0001	Customer
		//E001 0010	Access Code User
		//E001 0011	Access Code Operator
		//E001 0100	Access Code System Operator
		//E001 0101	Access Code Developer
		//E001 0110	Password
		else if ((c & 0x7F) == 0x17) {
			//	E001 0111	Error flags (binary)
			//	2 data bytes
			BOOST_ASSERT(length_ == 2);
			return STATE_DATA_7D_17;
		}
		//E001 1000	Error mask
		//E001 1001	Reserved

		//E001 1010	Digital Output (binary)
		//E001 1011	Digital Input (binary)
		//E001 1100	Baudrate [Baud]
		//E001 1101	response delay time [bittimes]
		//E001 1110	Retry
		//E001 1111	Reserved
		//Enhanced storage management
		//E010 0000	First storage # for cyclic storage
		//E010 0001	Last storage # for cyclic storage
		//E010 0010	Size of storage block
		//E010 0011	Reserved
		//E010 01nn	Storage interval [sec..day] 
		else if ((c & 0x7F) == 0x28) {
			//	E010 1000	Storage interval month(s)
			unit_ = mbus::MONTH;
		}
		else if ((c & 0x7F) == 0x29) {
			//	E010 1001	Storage interval year(s)
			unit_ = mbus::YEAR;
		}
		//E010 1010	Reserved
		else if (c == 0x2b) {
			//E010 1011	Reserved
			unit_ = mbus::SECOND;
		}
		//E010 11nn	Duration since last readout [sec..days] 
		//Enhanced tariff management
		//E011 0000	Start (date/time) of tariff  
		//E011 00nn	Duration of tariff (nn=01 ..11: min to days)
		//E011 01nn	Period of tariff [sec to days] 
		else if ((c & 0x7F) == 0x38) {
			//	E011 1000	Period of tariff months
			unit_ = mbus::MONTH;
		}
		else if ((c & 0x7F) == 0x39) {
			//	E011 1001	Period of tariff years
			unit_ = mbus::YEAR;
		}
		//E011 1010	dimensionless / no VIF
		//E011 1011	Reserved
		//E011 11xx	Reserved
		//electrical units
		else if ((c & 0x7F) == 0x40) {
			//	E100 nnnn	10nnnn-9 Volts
			scaler_ = (c & 0x0f) - 9;
			unit_ = mbus::VOLT;
		}
		else if ((c & 0x7F) == 0x50) {
			//	E101 nnnn	10nnnn-12 Ampere
			scaler_ = (c & 0x0f) - 12;
			unit_ = mbus::AMPERE;
		}
		//E110 0000	Reset counter
		//E110 0001	Cumulation counter
		//E110 0010	Control signal
		//E110 0011	Day of week
		//E110 0100	Week number
		//E110 0101	Time point of day change
		//E110 0110	State of parameter activation
		//E110 0111	Special supplier information
		//E110 10pp	Duration since last cumulation [hours..years]
		//E110 11pp	Operating time battery [hours..years]
		//E111 0000	Date and time of battery change
		else if ((c & 0x7F) == 0x74) {
			// E111 0100 remaining battery life time
			unit_ = mbus::DAY;
		}
		return STATE_DATA_;
	}


	vdb_reader::state vdb_reader::state_data(char c)
	{
		//
		//	collect data
		//
		buffer_.push_back(c);

		switch (length_) {
		case mbus::DFC_NO_DATA:
			//	no data 
			value_ = cyng::make_object();
			return STATE_COMPLETE;

		case mbus::DFC_8_BIT_INT://	8 Bit Integer
			BOOST_ASSERT(buffer_.size() == 1);
			value_ = cyng::make_object(c);
			return STATE_COMPLETE;

		case mbus::DFC_16_BIT_INT:	//	16 Bit Integer
			if (buffer_.size() == 2)	return make_i16_value();
			break;
		case mbus::DFC_24_BIT_INT:	//	24 Bit Integer
			if (buffer_.size() == 3)	return make_i24_value();
			break;
		case mbus::DFC_32_BIT_INT:	//	32 Bit Integer
			if (buffer_.size() == 4)	return make_i32_value();
			break;
		case mbus::DFC_32_BIT_REAL:	//	32 Bit Real
			//	ToDo: implement
			if (buffer_.size() == 4)	return STATE_COMPLETE;
			break;
		case mbus::DFC_48_BIT_INT:	//	48 Bit Integer
			//	ToDo: implement
			if (buffer_.size() == 6)	return STATE_COMPLETE;
			break;
		case mbus::DFC_64_BIT_INT:	//	64 Bit Integer
			//	ToDo: implement
			if (buffer_.size() == 8)	return STATE_COMPLETE;
			break;

		case mbus::DFC_READOUT:	//	Selection for Readout
			if (buffer_.size() == length_)	return STATE_COMPLETE;
			break;

		case mbus::DFC_2_DIGIT_BCD:	//	2 digit BCD
			if (buffer_.size() == 1)	return make_bcd_value(buffer_.size());
			break;
		case mbus::DFC_4_DIGIT_BCD:	// 	4 digit BCD
			if (buffer_.size() == 2)	return make_bcd_value(buffer_.size());
			break;
		case mbus::DFC_6_DIGIT_BCD:	//	5 digit BCD
			if (buffer_.size() == 3)	return make_bcd_value(buffer_.size());
			break;
		case mbus::DFC_8_DIGIT_BCD:	//	8 digit BCD
			if (buffer_.size() == 4)	return make_bcd_value(buffer_.size());
			break;

		case mbus::DFC_VAR:	// variable length - The length of the data is given with the first byte of data, which is here called LVAR.
			//	
			//	read variable length code
			//
			buffer_.clear();
			if (c < 0xC0) {
				//	max 192 characters
				length_ = c;
				return STATE_DATA_ASCII_;
			}
			else if (c < 0xD0) {
				length_ = (c - 0xC0) * 2;
				return STATE_DATA_BCD_POSITIVE_;
			}
			else if (c < 0xE0) {
				length_ = (c - 0xD0) * 2;
				return STATE_DATA_BCD_NEGATIVE_;
			}
			else if (c < 0xF0) {
				length_ = c - 0xE0;
				return STATE_DATA_BINARY_;
			}
			else if (c < 0xFB) {
				length_ = 8;
				return STATE_DATA_FP_;
			}
			//else reserved
			length_ = c - 0xFB;
			return STATE_COMPLETE;

		case mbus::DFC_12_DIGIT_BCD:	// 12 digit BCD
			if (buffer_.size() == 6)	return make_bcd_value(buffer_.size());
			break;
		case mbus::DFC_SPECIAL:	// special functions
			//	ToDo: implement
			if (buffer_.size() == length_)	return STATE_COMPLETE;
			break;

		default:
			break;
		}
		return state_;
	}

	vdb_reader::state vdb_reader::state_data_ascii(char c)
	{
		//
		//	collect data
		//
		buffer_.push_back(c);

		if (buffer_.size() == length_) {
			value_ = cyng::make_object(buffer_);
			return STATE_COMPLETE;
		}
		return state_;

	}

	vdb_reader::state vdb_reader::state_data_7d_08(char c)
	{
		//	E000 1000	Access Number(unique telegramm identification)
		buffer_.push_back(c);
		if (buffer_.size() == 2)	return make_i16_value();
		return state_;
	}

	vdb_reader::state vdb_reader::state_data_7d_17(char c)
	{
		//	E001 0111	Error flags (binary)
		buffer_.push_back(c);
		if (buffer_.size() == 2)	return make_i16_value();
		return state_;
	}

	void vdb_reader::decode_time_unit(std::uint8_t code)
	{
		//	0 = second
		//	1 = minutes
		//	2 = hours
		//	3 = days
		switch (code) {
		case 0:
			unit_ = mbus::SECOND;
			break;
		case 1:
			unit_ = mbus::MIN;
			break;
		case 2:
			unit_ = mbus::HOUR;
			break;
		case 3:
			unit_ = mbus::DAY;
			break;
		default:
			break;
		}
	}

	vdb_reader::state vdb_reader::make_i16_value()
	{
		BOOST_ASSERT(buffer_.size() == 2);

		if (date_flag_) {
			//
			//	format G
			//
			int day = (0x1f) & buffer_.at(0);
			int year1 = ((0xe0) & buffer_.at(0)) >> 5;
			int month = (0x0f) & buffer_.at(1);
			int year2 = ((0xf0) & buffer_.at(1)) >> 1;
			int year = (2000 + year1 + year2);

			//mbus::uformat_G fmt_g;
			//fmt_g.raw_[0] = buffer_.at(0);
			//fmt_g.raw_[1] = buffer_.at(1);

			//
			//	make date
			//
			value_ = cyng::make_object(cyng::chrono::init_tp(year, month, day, 0, 0, 0));
		}
		else {
			union {
				char inp_[2];
				std::uint16_t out_;
			} cnv;
			std::copy(buffer_.begin(), buffer_.end(), std::begin(cnv.inp_));
			value_ = cyng::make_object(cnv.out_);
		}
		return STATE_COMPLETE;
	}

	vdb_reader::state vdb_reader::make_i24_value()
	{
		BOOST_ASSERT(buffer_.size() == 3);
		if ((buffer_.at(2) & 0x80) == 0x80) {
			// negative
			std::int32_t val = buffer_.at(0)
				| (buffer_.at(1) << 8)
				| (buffer_.at(2) << 16)
				| 0xff << 24
				;
			value_ = cyng::make_object(val);
		}
		else {
			//	positive
			union {
				char inp_[3];
				std::uint32_t out_;
			} cnv;
			cnv.out_ = 0u;
			std::copy(buffer_.begin(), buffer_.end(), std::begin(cnv.inp_));
			value_ = cyng::make_object(cnv.out_);
		}
		return STATE_COMPLETE;
	}

	vdb_reader::state vdb_reader::make_i32_value()
	{
		BOOST_ASSERT(buffer_.size() == 4);
		if (date_flag_) {
			;	//	?
		}
		else if (date_time_flag_) {
			int min = (buffer_.at(0) & 0x3f);
			int hour = (buffer_.at(1) & 0x1f);
			int yearh = (0x60 & buffer_.at(1)) >> 5;
			int day = (buffer_.at(2) & 0x1f);
			int year1 = (0xe0 & buffer_.at(2)) >> 5;
			int month = (buffer_.at(3) & 0x0f);
			int year2 = (0xf0 & buffer_.at(3)) >> 1;

			if (yearh == 0) {
				yearh = 1;
			}

			int year = 1900 + 100 * yearh + year1 + year2;

			//mbus::uformat_F fmt_f;
			//fmt_f.raw_[0] = buffer_.at(0);
			//fmt_f.raw_[1] = buffer_.at(1);
			//fmt_f.raw_[2] = buffer_.at(2);
			//fmt_f.raw_[3] = buffer_.at(3);

			//
			//	make date time
			//
			value_ = cyng::make_object(cyng::chrono::init_tp(year, month, day, hour, min, 0));

		}
		else {
			union {
				char inp_[4];
				std::uint32_t out_;
			} cnv;
			std::copy(buffer_.begin(), buffer_.end(), std::begin(cnv.inp_));
			value_ = cyng::make_object(cnv.out_);
		}
		return STATE_COMPLETE;
	}

	vdb_reader::state vdb_reader::make_bcd_value(std::size_t size)
	{
		BOOST_ASSERT(buffer_.size() == size);
		switch (size) {
		case 1:
			value_ = cyng::make_object(mbus::bcd_to_n<std::uint8_t>(buffer_));
			break;
		case 2:
			value_ = cyng::make_object(mbus::bcd_to_n<std::uint16_t>(buffer_));
			break;
		case 3:
			//	ToDo: convert
			value_ = cyng::make_object(buffer_);
			break;
		case 4:
			value_ = cyng::make_object(mbus::bcd_to_n<std::uint32_t>(buffer_));
			break;
		case 6:
			//	ToDo: convert
			value_ = cyng::make_object(buffer_);
			break;
		default:
			value_ = cyng::make_object(buffer_);
			break;
		}

		return STATE_COMPLETE;
	}

}	//	node