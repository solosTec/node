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
#include "test-ipt-004.h"
#include "test-ipt-005.h"

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
BOOST_AUTO_TEST_CASE(ipt_004)
{
	//
	//	test data transfer
	//
	using namespace node;
	BOOST_CHECK(test_ipt_004());
}
BOOST_AUTO_TEST_CASE(ipt_005)
{
	//
	//	test data push
	//
	using namespace node;
	BOOST_CHECK(test_ipt_005());
}
BOOST_AUTO_TEST_SUITE_END()	//	IPT


#include "test-sml-001.h"
#include "test-sml-002.h"
#include "test-sml-003.h"
#include "test-sml-004.h"
//#include "test-sml-005.h"

BOOST_AUTO_TEST_SUITE(SML)
BOOST_AUTO_TEST_CASE(sml_001)
{
	using namespace node;
	BOOST_CHECK(test_sml_001());
}
BOOST_AUTO_TEST_CASE(sml_002)
{
	using namespace node;
	BOOST_CHECK(test_sml_002());
}
BOOST_AUTO_TEST_CASE(sml_003)
{
	using namespace node;
	BOOST_CHECK(test_sml_003());
}
BOOST_AUTO_TEST_CASE(sml_004)
{
	using namespace node;
	BOOST_CHECK(test_sml_004());
}
//BOOST_AUTO_TEST_CASE(sml_005)
//{
//	using namespace node;
//	BOOST_CHECK(test_sml_005());
//}
BOOST_AUTO_TEST_SUITE_END()	//	SML


#include "test-mbus-001.h"
#include "test-mbus-002.h"
#include "test-mbus-003.h"

BOOST_AUTO_TEST_SUITE(MBUS)
BOOST_AUTO_TEST_CASE(mbus_001)
{
	using namespace node;
	BOOST_CHECK(test_mbus_001());
}
BOOST_AUTO_TEST_CASE(mbus_002)
{
	using namespace node;
	BOOST_CHECK(test_mbus_002());
}
BOOST_AUTO_TEST_CASE(mbus_003)
{
	using namespace node;
	BOOST_CHECK(test_mbus_003());
}
BOOST_AUTO_TEST_SUITE_END()	//	MBUS
