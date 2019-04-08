/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-004.h"
#include <NODE_project_info.h>
//#include <smf/mbus/parser.h>
//#include <smf/mbus/header.h>
//#include <smf/sml/srv_id_io.h>
//#include <smf/mbus/defs.h>
//#include <smf/mbus/aes.h>
#include <smf/mbus/variable_data_block.h>
//
//#include <cyng/io/serializer.h>
#include <cyng/io/io_buffer.h>
//#include <cyng/tuple_cast.hpp>
//
//#include <iostream>
//#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace node 
{
	bool test_mbus_004()
	{
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
