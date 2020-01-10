/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 
#include "test-sml-006.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/sml/parser/obis_parser.h>

namespace node 
{

	bool test_sml_006()
	{
		//
		//	test OBIS parser
		//
		auto r = sml::parse_obis("1-2:3.4.5*255");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 1);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 2);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 3);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 4);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 5);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);

		r = sml::parse_obis("010203040506");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 1);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 2);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 3);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 4);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 5);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 6);

		r = sml::parse_obis("F.F");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 0);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 0);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 97);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 97);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 0);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);

		r = sml::parse_obis("1.2.3");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 0);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 0);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 1);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 2);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 3);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);
		
		r = sml::parse_obis("C.2.3");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 0);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 0);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 96);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 2);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 3);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);
		
		r = sml::parse_obis("F.2.3");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 0);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 0);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 97);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 2);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 3);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);

		r = sml::parse_obis("L.2.3");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 0);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 0);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 98);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 2);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 3);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);

		r = sml::parse_obis("P.2.3");
		BOOST_CHECK(r.second);
		BOOST_CHECK_EQUAL(r.first.size(), 6);
		BOOST_CHECK_EQUAL(r.first.get_medium(), 0);
		BOOST_CHECK_EQUAL(r.first.get_channel(), 0);
		BOOST_CHECK_EQUAL(r.first.get_indicator(), 99);
		BOOST_CHECK_EQUAL(r.first.get_mode(), 2);
		BOOST_CHECK_EQUAL(r.first.get_quantities(), 3);
		BOOST_CHECK_EQUAL(r.first.get_storage(), 255);

		return true;
	}

}
