/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-sml-001.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/sml/protocol/parser.h>
#include <cyng/io/serializer.h>
//#include <boost/random.hpp>
 
namespace node 
{
	bool test_sml_001()
	{
		//boost::random::mt19937 rng;
		//boost::random::uniform_int_distribution<std::uint32_t> distribution;

		//for (std::size_t idx = 0; idx < 100; idx++)
		//{
		//	std::cout << std::hex << distribution(rng) << ' ';
		//}

		sml::parser p([](cyng::vector_t&& prg) {
			cyng::vector_t r = std::move(prg);
			std::cout << cyng::io::to_str(r) << std::endl;
		}, true);

		//std::ifstream ifile("push-data-3890346734-4161255391.bin", std::ios::binary | std::ios::app);
		std::ifstream ifile("C:\\projects\\workplace\\node\\Debug\\push-data-3586334585-418932835.bin", std::ios::binary | std::ios::app);
		if (ifile.is_open())
		{
			//	dont skip whitepsaces
			ifile >> std::noskipws;
			p.read(std::istream_iterator<std::uint8_t>(ifile), std::istream_iterator<uint8_t>());

		}
		
		return true;
	}

}
