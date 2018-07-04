/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-sml-002.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/sml/protocol/parser.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/protocol/value.hpp>

#include <cyng/io/serializer.h>
#include <cyng/factory.h>

namespace node 
{
	bool test_sml_002()
	{
		cyng::tuple_t tpl;
		//for (std::size_t idx = 0; idx < 0x32; idx++)
		//{
		//	tpl.push_back(cyng::make_now());
		//}

		std::string s("1234567890abcdef1234567890abcdef");
		tpl.push_back(cyng::make_object("1234567890abcdef1234567890abcdef"));

		{
			std::ofstream ofile("test.bin", std::ios::binary | std::ios::trunc);
			if (ofile.is_open())
			{
				sml::serialize(ofile, cyng::make_object(tpl));
			}
			//	ofile closed
		}
		{
			std::ifstream ifile("test.bin", std::ios::binary | std::ios::app);
			if (ifile.is_open())
			{
				sml::parser p([](cyng::vector_t&& prg) {
					cyng::vector_t r = std::move(prg);
					std::cout << cyng::io::to_str(r) << std::endl;
				}, true, false);

				ifile >> std::noskipws;
				p.read(std::istream_iterator<std::uint8_t>(ifile), std::istream_iterator<uint8_t>());
			}
		}


		return true;
	}

}
