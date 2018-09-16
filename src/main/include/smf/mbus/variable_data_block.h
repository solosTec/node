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
#include <cyng/intrinsics/buffer.h>
#include <cyng/object.h>

namespace node 
{
	/*
	 * Each variable data block has three elements:
	 * <ol>
	 * <li>data information block (DIB)</li>
	 * <li>value information block (VIB)</li>
	 * <li>data field (DF)</li>
	 * </ol>
	 */
	class variable_data_block
	{
	public:
		variable_data_block();
		
		std::size_t decode(cyng::buffer_t const&, std::size_t offset);

		cyng::object const& get_value() const;
		std::int8_t get_scaler() const;

		
	private:
		std::size_t finalize_dib(cyng::buffer_t const&, std::size_t offset);
		std::size_t decode_vif(cyng::buffer_t const&, std::size_t offset);
		std::size_t decode_data(cyng::buffer_t const&, std::size_t offset);
		std::size_t set_BCD(cyng::buffer_t const& data, std::size_t offset, std::size_t range);
		
		std::string decode_main_vif(std::uint8_t);
		std::string decode_main_ext_vif(std::uint8_t vif);
		
		void decodeTimeUnit(std::uint8_t vif);
		
		std::string read_e0(std::uint8_t vif);
		std::string read_e1(std::uint8_t vif);
		std::string read_01(std::uint8_t vif);
		std::string read_e00(std::uint8_t vif);
		std::string read_e10(std::uint8_t vif);
		std::string read_e11(std::uint8_t vif);
		std::string read_e110(std::uint8_t vif);
		std::string read_e111(std::uint8_t vif);
		
	private:
		std::uint8_t length_;
		std::uint64_t storage_nr_;	//!< max value 0x20000000000
		std::uint32_t tariff_;	//!< max value 0x100000000
		std::uint16_t sub_unit_;	//	max value 0x10000
		std::int8_t scaler_;
		mbus::unit_code	unit_;
		bool dt_flag_date_, dt_flag_time_;
		cyng::object value_;
	};
	
}	//	node

#endif // NODE_MBUS_VARIABLE_DATA_BLOCK_H
