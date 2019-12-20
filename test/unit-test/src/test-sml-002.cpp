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
		//
		//	test SML serialization/deserialization of long strings/octets
		//


		std::string const file_name{ "test.bin" };
		cyng::tuple_t tpl = cyng::tuple_factory("1234567890abcdef.1234567890abcdef.1234567890abcdef");

		{
			std::ofstream ofile(file_name, std::ios::binary | std::ios::trunc);
			BOOST_CHECK(ofile.is_open());
			if (ofile.is_open())
			{
				sml::serialize(ofile, cyng::make_object(tpl));
			}
			//	ofile closed
		}
		{
			std::ifstream ifile(file_name, std::ios::binary | std::ios::in);
			BOOST_CHECK(ifile.is_open());
			if (ifile.is_open())
			{
				sml::parser p([](cyng::vector_t&& prg) {
					cyng::vector_t r = std::move(prg);
					//std::cout << cyng::io::to_str(r) << std::endl;
				}, true, false, false);

				ifile.unsetf(std::ios_base::skipws);
				p.read(std::istream_iterator<std::uint8_t>(ifile), std::istream_iterator<uint8_t>());
			}
		}


		return true;
	}

}
