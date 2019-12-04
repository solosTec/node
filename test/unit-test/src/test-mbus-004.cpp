/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-004.h"
#include <NODE_project_info.h>
#include <smf/mbus/variable_data_block.h>
//#include <smf/mbus/dif.h>

#include <cyng/io/io_buffer.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace node 
{
	bool test_mbus_004()
	{
		{
			std::uint8_t const val = 0x14;
			std::int8_t scaler_ = static_cast<std::int8_t>(val & 0x07) - 6; 
			scaler_ = -2;

			//mbus::udif d;
			//d.raw_ = 0xE8;	//	1110 1000
			//auto length_ = d.dif_.code_;

			////
			////	see function_field_code
			////
			////	0 - instantaneous value
			////	1 - maximum value
			////	2 - minimum value
			////	all other values signal an error
			////
			////
			//auto function_field_ = d.dif_.ff_;
			//BOOST_ASSERT(function_field_ != 3);

			////
			////	storage number
			////
			//auto storage_nr_ = d.dif_.sn_;

			//auto ext = d.dif_.ext_;

			//
			//	{03 74} [14 00] 00 {04 14}[dbe618]00 {44 14}[dbe618] 00 {42 6c}7e2b02fd7442120f0100c8
			//	1631963 - 2 m3(‭18E6DB‬) m3 == 13dez == 0Dhex
			//
			auto inp = cyng::make_buffer({ 0x03, 0x74, 0x14, 0x00, 0x00, 0x04, 0x14, 0xdb, 0xe6, 0x18, 0x00, 0x44, 0x14, 0xdb, 0xe6, 0x18, 0x00, 0x42, 0x6c, 0x7e, 0x2b, 0x02, 0xfd, 0x74, 0x42, 0x12, 0x0f, 0x01, 0x00, 0xc8 });
			vdb_reader reader;
			std::size_t offset{ 0 };
			while (offset < inp.size()) {
				offset = reader.decode(inp, offset);
			}
		}

		{
			//
			//	68966.1 kWh
			//	
			auto inp = cyng::make_buffer({ 0x04, 0x05, 0xFD, 0x85, 0x0A, 0x00, 0x02, 0xFD, 0x08, 0x0B, 0x16 });

			vdb_reader reader;
			std::size_t offset{ 0 };
			//offset = reader.decode(inp, offset);
		}
		{
			//
			//	Value 68966.1 kWh
			//
			auto inp = cyng::make_buffer({ 0x04, 0x05, 0xFD, 0x85, 0x0A, 0x00, 0x02, 0xFD, 0x08, 0x0B, 0x16 });

			vdb_reader reader;
			std::size_t offset{ 0 };
			//offset = reader.decode(inp, offset);

		}
		{
			//
			//	data from gas meter 87654329
			//
			auto inp = cyng::make_buffer({ 0x03, 0x74, 0x82, 0x00, 0x00, 0x04, 0x94, 0x3a, 0x72, 0x00, 0x00, 0x00, 0x44, 0x94, 0x3a, 0x5d, 0x00, 0x00, 0x00, 0x42, 0x6c, 0x7f, 0x23, 0x02, 0xfd, 0x74, 0x7b, 0xff, 0x2f, 0x2f });

			//	meter with 1.30 => 0x82
			//29 43 65 87 e6 1e bf 03 c4 00 20 05 d54e2444b2c5a03c9273feaf66cfbe73cb58c0e0ba8bbd443f5af3f8886100c3
			//auto inp = cyng::make_buffer({ 0xd5, 0x4e, 0x24, 0x44, 0xb2, 0xc5, 0xa0, 0x3c, 0x92, 0x73, 0xfe, 0xaf, 0x66, 0xcf, 0xbe, 0x73, 0xcb, 0x58, 0xc0, 0xe0, 0xba, 0x8b, 0xbd, 0x44, 0x3f, 0x5a, 0xf3, 0xf8, 0x88, 0x61, 0x0, 0x0c3 });
			vdb_reader reader;
			std::size_t offset{ 0 };
			//offset = reader.decode(inp, offset);

		}
		{
			//
			//	from OMS-Spec_Vol2_AnnexN_A032.pdf
			//	N.1.1 wM-Bus Meter with Security profile A
			//
			auto inp = cyng::make_buffer({ 0x0c, 0x14, 0x27, 0x04, 0x85, 0x02, 0x04, 0x6D, 0x32, 0x37, 0x1F, 0x15, 0x02, 0xFD, 0x17, 0x00, 0x00 });

			vdb_reader reader;
			std::size_t offset{ 0 };
			//offset = reader.decode(inp, offset);
		}

		{
			//
			//	variable length
			//
			auto inp = cyng::make_buffer({/* 0x6C, 0x81, 0x11,*/ 0x8C, 0x04, 0x13, 0x90, 0x52, 0x34, 0x00, 0x8D, 0x04, 0x93, 0x1F, 0x23, 0xFB, 0xFE, 0x60, 0x26, 0x00, 0x39, 0x39, 0x00, 0x34, 0x31, 0x00, 0x68, 0x34, 0x00, 0x10, 0x42, 0x00, 0x78});

			vdb_reader reader;
			std::size_t offset{ 0 };
			offset = reader.decode(inp, offset);
		}

		return true;
	}

}
