/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-ipt-002.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <smf/ipt/parser.h>
#include <smf/ipt/scramble_key_io.hpp>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <type_traits>
#include <typeinfo>
//#include <boost/core/demangle.hpp>

namespace node 
{
	//template <typename T>
	//void test(T const& v)
	//{
	//	char const * name_1 = typeid(T).name();
	//	std::cout << name_1 << std::endl; // prints 1XIiE
	//	std::cout << boost::core::demangle(name_1) << std::endl; // prints X<int>

	//	using type = typename std::decay<T>::type;
	//	char const * name_2 = typeid(type).name();
	//	std::cout << name_2 << std::endl; // prints 1XIiE
	//	std::cout << boost::core::demangle(name_2) << std::endl; // prints X<int>

	//}

	bool test_ipt_002()
	{
		//
		//	std::decay converts char [6] to char * __ptr64
		//
		//test("hello");
		//test(std::string("hello"));

		//
		//	1. test parsing a scrambled login command
		//

		ipt::scramble_key sk = ipt::gen_random_sk();
		cyng::vector_t result;
		ipt::parser p([&result](cyng::vector_t&& prg) {
			//	[op:ESBA,,ipt.set.sk,op:INVOKE,op:REBA,op:ESBA,name,pwd,,ipt.req.login.scrambled,op:INVOKE,op:REBA]
			//std::cout << cyng::io::to_str(prg) << std::endl;
			result = std::move(prg);
		}, sk);


		//	scrambled login
		std::vector<unsigned char> inp{
			0x02, 0xc0,	//	cmd
			0x00,		// sequence
			0x00,		//	reserved
			0x31, 0x00, 0x00, 0x00,	//	length = header + data
			'n', 'a', 'm', 'e', '\0',
			'p', 'w', 'd', '\0'
		};
		inp.insert(inp.end(), sk.key().begin(), sk.key().end());
		p.read(inp.begin(), inp.end());
		BOOST_CHECK_EQUAL(result.size(), 12);
		BOOST_CHECK(cyng::value_cast(result.at(1), ipt::gen_random_sk()).key() == sk.key());

		//
		//	2. test stream operator for ipt::scramble_k
		//

		//using ipt::operator>>;
		std::stringstream ss;
		ss << "AABBCCDDEEFF0708090001020304050607080900010203040506070809000001";
		ss >> sk;
		return true;
	}

}
