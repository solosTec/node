#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/mbus/server_id.h>

#include <cyng/io/ostream.h>

#include <iostream>
#include <fstream>


BOOST_AUTO_TEST_SUITE(mbus_suite)

BOOST_AUTO_TEST_CASE(id)
{
	//	Example: 0x3105c => 96072000
	auto const m1 = smf::to_meter_id(0x3105c);
	BOOST_REQUIRE_EQUAL(m1.size(), 4);
	BOOST_REQUIRE_EQUAL(m1.at(0) & 0xFF, 0x96);
	BOOST_REQUIRE_EQUAL(m1.at(1) & 0xFF, 0x07);
	BOOST_REQUIRE_EQUAL(m1.at(2) & 0xFF, 0x20);
	BOOST_REQUIRE_EQUAL(m1.at(3) & 0xFF, 0x00);
	//s.cpp(22) : fatal error : in "mbus_suite/id" : critical check m1.at(0) == 0x96 has failed[0xffffff96 != 0x96]

	//	 "10320047" = > [10, 32, 00, 47]dec
	auto const m2 = smf::to_meter_id("10320047");
	BOOST_REQUIRE_EQUAL(m2.size(), 4);
	BOOST_REQUIRE_EQUAL(m2.at(0) & 0xFF, 0x0a);
	BOOST_REQUIRE_EQUAL(m2.at(1) & 0xFF, 0x20);
	BOOST_REQUIRE_EQUAL(m2.at(2) & 0xFF, 0x00);
	BOOST_REQUIRE_EQUAL(m2.at(3) & 0xFF, 0x2f);

	auto const s1 = smf::srv_id_to_str(std::array<char, 9>({ 0x02, (char)0xe6, 0x1e, 0x03, 0x19, 0x77, 0x15, 0x3c, 0x07 }));
	BOOST_REQUIRE_EQUAL(s1, "02-e61e-03197715-3c-07");
}

BOOST_AUTO_TEST_SUITE_END()