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

namespace node 
{
	bool test_sml_004()
	{
		sml::obis code(1, 2, 3, 4, 5, 6);
		BOOST_CHECK(code.match({1, 2}));
		BOOST_CHECK(! code.match({2, 3, 4}));
		return true;
	}

}
