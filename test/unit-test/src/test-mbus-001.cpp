/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-001.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/mbus/defs.h>

namespace node 
{
	bool test_mbus_001()
	{
		//	0x0442
		BOOST_CHECK_EQUAL(sml::encode_id("ABB"), 0x0442);
		BOOST_CHECK_EQUAL(sml::encode_id("ABB"), sml::encode_id("abb"));

		return true;
	}

}
