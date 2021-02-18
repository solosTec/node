/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/scramble_key_format.h>
#include <smf/ipt/scramble_key_io.hpp>
#include <iomanip>
#include <sstream>
#include <boost/assert.hpp>

namespace smf
{
	namespace ipt
	{
		
		std::string to_string(scramble_key const& v)
		{
			std::ostringstream ss;
			ss << v;
			return ss.str();
		}

		scramble_key to_sk(std::string const& v)
		{
			scramble_key sk;
			std::stringstream ss;
			ss << v;
			ss >> sk;
			return sk;
		}

	}	//	ipt
}	//	node



