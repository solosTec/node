/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-002.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/mbus/parser.h>
#include <cyng/io/serializer.h>

namespace node 
{
	bool test_mbus_002()
	{
		//	initialize meter
		//	10 40 fe 3e 16 
		{
			auto const inp = cyng::make_buffer({ 0x10, 0x40, 0xfe, 0x3e, 0x16 });
			mbus::parser p([](cyng::vector_t&& prg) {
				std::cout << cyng::io::to_str(prg) << std::endl;
			});
			p.read(inp.begin(), inp.end());
		}

		//	change baudrate to 9600
		//	 68 03 03 68 53 01 BD 11 16
		{
			auto const inp = cyng::make_buffer({ 0x68, 0x03, 0x03, 0x68, 0x53, 0x01, 0xBD, 0x11, 0x16 });
			mbus::parser p([](cyng::vector_t&& prg) {
				std::cout << cyng::io::to_str(prg) << std::endl;
			});
			p.read(inp.begin(), inp.end());
		}

		//	long frame
		//	68 1B 1b 68 08 01 72 16 10 00 12 e6 1e 36 07  11 68 1b 1b 68 08 2d 32 55 25 18 0d 66 5e 3c 07 1a 16  00 00 0c 78 55 01 00 19 0c 13 26 00 00 00 7b 16
		{
			auto const inp = cyng::make_buffer({ 0x68, 0x1B, 0x1B, 0x68, 0x08, 0x2d, 0x32, 0x55, 0x25, 0x18, 0x0d, 0x66, 0x5e, 0x3c, 0x07, 0x34, 0x16, 0x00, 0x00, 0x0c, 0x78, 0x55, 0x01, 0x00, 0x19, 0x0c, 0x13, 0x26, 0x00, 0x00, 0x00, 0x75, 0x16 });
			mbus::parser p([](cyng::vector_t&& prg) {
				std::cout << cyng::io::to_str(prg) << std::endl;
				});
			p.read(inp.begin(), inp.end());
		}

		//	long frame
		//	68 1B 1B 68 08 01 72 16 10 00 12 E6 1E 36 07  04 B4 86 86 CA 7A D6 39 54 49 31 96 73 5E 3C 80 2D 16
		{
			auto const inp = cyng::make_buffer({ 0x68, 0x1B, 0x1B, 0x68, 0x08, 0x01, 0x72, 0x16, 0x10, 0x00, 0x12, 0xE6, 0x1E, 0x36, 0x07, 0x04, 0xB4, 0x86, 0x86, 0xCA, 0x7A, 0xD6, 0x39, 0x54, 0x49, 0x31, 0x96, 0x73, 0x5E, 0x3C, 0x80, 0x2D, 0x16 });
			mbus::parser p([](cyng::vector_t&& prg) {
				std::cout << cyng::io::to_str(prg) << std::endl;
				});
			p.read(inp.begin(), inp.end());
		}

		return true;
	}

}
