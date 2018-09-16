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
#include <smf/mbus/variable_data_block.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>

namespace node 
{
	bool test_mbus_003()
	{

		//auto tpl = cyng::tuple_cast<
		//	std::string,			//	[0] manufacturer
		//	std::uint8_t,			//	[1] version
		//	std::uint8_t,			//	[2] media
		//	std::uint32_t,			//	[3] device ID
		//	std::uint8_t,			//	[4] frame type
		//	cyng::buffer_t			//	[5] data
		//>(frame);

		//	get data
		auto p = std::string("C:\\projects\\github\\node\\test\\unit-test\\src\\samples\\mbus-003.bin");
		//auto p = std::string("C:\\projects\\github\\node\\test\\unit-test\\src\\samples\\mbus-004.bin");
		//auto p = std::string("C:\\projects\\github\\node\\test\\unit-test\\src\\samples\\mbus-005.bin");
		std::ifstream ifs(p, std::ios::binary | std::ios::app);
		if (ifs.is_open())
		{
			ifs >> std::noskipws;
			cyng::buffer_t data;
			data.insert(data.begin(), std::istream_iterator<char>(ifs), std::istream_iterator<char>());

			wmbus::parser p([](cyng::vector_t&& prg) {

				//	[op:ESBA,GWF,0,e,0009b5f8,72,13090016E61E3C07430020654C96AED902C8485C9E381D5AF2791A2818DF3763271C9EB2C29321E5EAC458C7,mbus.push.frame,op:INVOKE,op:REBA]
				std::cout << cyng::io::to_str(prg) << std::endl;

				cyng::buffer_t data;
				data = cyng::value_cast<>(prg.at(6), data);

				std::size_t offset{ 0 };
				while (offset < data.size()) {
					variable_data_block vdb;
					offset = vdb.decode(data, offset);
					std::cout << "#" << offset << ", value: " << cyng::io::to_str(vdb.get_value()) << ", scaler: " << +vdb.get_scaler() << std::endl;
				}

			});
			p.read(data.begin(), data.end());
			//p.reset();
		}

		return true;
	}

}
