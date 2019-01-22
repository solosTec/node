/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-ipt-001.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <smf/ipt/parser.h>
#include <cyng/io/serializer.h>

namespace node 
{
	bool test_ipt_001()
	{
		//
		//	test parsing a public login command
		//

		ipt::scramble_key sk;
		cyng::vector_t result;
		ipt::parser p([&result](cyng::vector_t&& prg) {
			//std::cout << cyng::io::to_str(prg) << std::endl;
			result = std::move(prg);
		}, sk);

		//	public login
		std::vector<unsigned char> inp{
			0x01, 0xc0,	//	cmd
			0x00,		//	sequence
			0x00,		//	reserved
			0x11, 0x00, 0x00, 0x00,	//	length = header + data
			'n', 'a', 'm', 'e', '\0',
			'p', 'w', 'd', '\0'
		};
		p.read(inp.begin(), inp.end());
		BOOST_CHECK_EQUAL(result.size(), 6);

		return true;
	}

}
