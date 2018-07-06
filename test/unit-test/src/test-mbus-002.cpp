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
			cyng::buffer_t inp{ 0x10, 0x40, (char)0xfe, 0x3e, 0x16 };
			mbus::parser p([](cyng::vector_t&& prg) {
				std::cout << cyng::io::to_str(prg) << std::endl;
			});
			p.read(inp.begin(), inp.end());
		}

		//	change baudrate to 9600
		//	 68 03 03 68 53 01 BD 11 16
		{
			cyng::buffer_t inp{ 0x68, 0x03, 0x03, 0x68, 0x53, 0x01, (char)0xBD, 0x11, 0x16 };
			mbus::parser p([](cyng::vector_t&& prg) {
				std::cout << cyng::io::to_str(prg) << std::endl;
			});
			p.read(inp.begin(), inp.end());
		}

		return true;
	}

}
