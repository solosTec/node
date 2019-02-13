/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#include "test-serial-001.h"
#include <smf/serial/baudrate.h>
#include <boost/test/unit_test.hpp>

namespace node 
{
	bool test_serial_001()
	{
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(1u), 50u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(49u), 50u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(52u), 50u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(9000u), 9600u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(9600u), 9600u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(9601u), 9600u);

		BOOST_CHECK_EQUAL(serial::adjust_baudrate(57598u), 57600u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(57600u), 57600u);
		BOOST_CHECK_EQUAL(serial::adjust_baudrate(57602u), 57600u);

		return true;
	}

}
