/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-sml-004.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>

namespace node 
{
	bool test_sml_004()
	{
		//
		//	test obis and server id functions
		//

		sml::obis code(1, 2, 3, 4, 5, 6);
		BOOST_CHECK(code.match({1, 2}));
		BOOST_CHECK(! code.match({2, 3, 4}));

		
		std::string serial = sml::get_serial("02-e61e-03197715-3c-07");
		BOOST_CHECK_EQUAL(serial, "15771903");
		return true;
	}

}
