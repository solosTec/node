/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_MBUS_VARIABLE_DATA_BLOCK_H
#define NODE_MBUS_VARIABLE_DATA_BLOCK_H


#include <smf/mbus/defs.h>
#include <smf/mbus/units.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/intrinsics/buffer.h>
#include <cyng/object.h>
#include <array>

namespace node 
{
	/*
	 * Each variable data block has three elements:
	 * <ol>
	 * <li>data information block (DIB)</li>
	 *	<ol>
	 *	<li>data information field (DIF) (1 byte)</li>
	 *	<li>extended (DIFE) (0 .. 10 byte)</li>
	 *	</ol>
	 * <li>value information block (VIB)</li>
	 *	<ol>
	 *	<li>value information field (VIF) (1 byte)</li>
	 *	<li>extended (VIFE) (0 .. 10 byte)</li>
	 *	</ol>
	 * <li>data field (DF) (0 ... N byte)</li>
	 * </ol>
	 */
	class vdb_reader
	{
	public:
		vdb_reader(cyng::buffer_t const& server_id);
		std::size_t decode(cyng::buffer_t const&, std::size_t offset);

		/**
		 * @return value of data block
		 */
		cyng::object get_value() const;

		std::int8_t get_scaler() const;
		mbus::units	get_unit() const;

		bool is_valid() const;
		sml::obis get_code() const;

	private:
		enum class state {
			STATE_ERROR_,
			STATE_DIF_,
			STATE_DIF_EXT_,
			STATE_VIF_,
			STATE_VIF_EXT_,
			STATE_VIF_EXT_7B,
			STATE_VIF_EXT_7D,
			STATE_DATA_,
			STATE_DATA_ASCII_,
			STATE_DATA_BCD_POSITIVE_,	//	2 bytes
			STATE_DATA_BCD_NEGATIVE_,	//	2 bytes
			STATE_DATA_BINARY_,
			STATE_DATA_FP_,		//	floating point
			STATE_DATA_7D_08,	//	E000 1000	Access Number(transmission count)
			STATE_DATA_7D_17,	//	E001 0111	Error flags (binary)
			STATE_COMPLETE,	//	ready for next record
		} state_;

	private:
		state state_dif(char);
		state state_dif_ext(char);
		state state_vif(char);

		/**
		 * implements table 28 of DIN EN 13757-3:2013
		 */
		state state_vif_ext(char);
		state state_vife_7b(char);
		state state_vife_7d(char);
		void decode_time_unit(std::uint8_t);
		state state_data(char);
		state state_data_ascii(char);
		state state_data_7d_08(char);
		state state_data_7d_17(char);

		state make_i16_value();
		state make_i24_value();
		state make_i32_value();
		state make_bcd_value(std::size_t);

		void reset();
		std::uint8_t get_medium() const;

	private:
		cyng::buffer_t const server_id_;
		std::uint8_t length_;
		std::uint8_t function_field_;

		/**
		 * The storage number 0 signals an actual value.
		 */
		std::uint64_t storage_nr_;	//!< max value 0x20000000000

		/**
		 * Tariff 0 is usually the sum of all other tariffs.
		 */
		std::uint32_t tariff_;	//!< max value 0x100000000
		std::uint16_t sub_unit_;	//	max value 0x10000
		std::int8_t scaler_;

		/**
		 * unit of the data value.
		 */
		mbus::units	unit_;
		bool date_flag_;
		bool date_time_flag_;
		cyng::object value_;

		//
		//	temporary values
		//
		cyng::buffer_t buffer_;	//	bcd, etc...
		typename sml::obis::data_type code_;	//	std::array< std::uint8_t, 6 >

		/**
		 * If parser was successfull, we have valid data
		 */
		bool valid_;
	};

}	//	node

#endif // NODE_MBUS_VARIABLE_DATA_BLOCK_H
