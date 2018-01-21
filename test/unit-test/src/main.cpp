/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#define BOOST_TEST_MODULE CYNG
#include <boost/test/unit_test.hpp>
#include <cyng/cyng.h>

#include "test-ipt-001.h"
#include "test-ipt-002.h"
#include "test-ipt-003.h"

//	Start with:
//	./unit_test --report_level=detailed
//	./unit_test --run_test=IPT/ipt_001

BOOST_AUTO_TEST_SUITE(IPT)
BOOST_AUTO_TEST_CASE(ipt_001)
{
	using namespace node;
	BOOST_CHECK(test_ipt_001());
}
BOOST_AUTO_TEST_CASE(ipt_002)
{
	using namespace node;
	BOOST_CHECK(test_ipt_002());
}
BOOST_AUTO_TEST_CASE(ipt_003)
{
	using namespace node;
	BOOST_CHECK(test_ipt_003());
}
BOOST_AUTO_TEST_SUITE_END()	//	IPT

