/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-ipt-003.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <smf/ipt/parser.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>

namespace node 
{
	bool test_ipt_003()
	{
		//
		//	test parsing a public login command followed immediately
		//	by a protocol version request
		//

		ipt::scramble_key sk = ipt::gen_random_sk();
		cyng::vector_t result;
		ipt::parser p([&result](cyng::vector_t&& prg) {
			//	[op:ESBA,op:IDENT,name,pwd,ipt.req.login.public,op:INVOKE,op:REBA]
			//	[op:ESBA,op:IDENT,1,ipt.req.protocol.version,op:INVOKE,op:REBA]
			std::cout << cyng::io::to_str(prg) << std::endl;
			result = std::move(prg);
		}, sk);


		//	public login
		std::vector<unsigned char> inp{
			0x01, 0xc0,	//	cmd
			0x00,		//	sequence
			0x00,		//	reserved
			0x11, 0x00, 0x00, 0x00,	//	length = header + data
			'n', 'a', 'm', 'e', '\0',
			'p', 'w', 'd', '\0',
			0x1b,	//	escape
			0x00, 0xa0,	//	protocol version request
			0x01,		//	sequence
			0x00,		//	reserved
			0x08, 0x00, 0x00, 0x00,	//	length = header + data
		};
		p.read(inp.begin(), inp.end());
		return true;
	}

}
