/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-003.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <smf/mbus/parser.h>
#include <cyng/io/serializer.h>

namespace node 
{
	bool test_mbus_003()
	{
		//	get data
		auto p = std::string("C:\\projects\\github\\node\\test\\unit-test\\src\\samples\\mbus-003.bin");
		//auto p = std::string("C:\\projects\\github\\node\\test\\unit-test\\src\\samples\\mbus-004.bin");
		std::ifstream ifs(p, std::ios::binary | std::ios::app);
		if (ifs.is_open())
		{
			ifs >> std::noskipws;
			cyng::buffer_t data;
			data.insert(data.begin(), std::istream_iterator<char>(ifs), std::istream_iterator<char>());

			wmbus::parser p([](cyng::vector_t&& prg) {

				//	[op:ESBA,GWF,0,e,0009b5f8,72,13090016E61E3C07430020654C96AED902C8485C9E381D5AF2791A2818DF3763271C9EB2C29321E5EAC458C7,mbus.push.frame,op:INVOKE,op:REBA]
				std::cout << cyng::io::to_str(prg) << std::endl;
			});
			p.read(data.begin(), data.end());
			//p.reset();
		}

		return true;
	}

}
