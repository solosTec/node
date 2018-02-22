/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/scramble_key_format.h>
#include <iomanip>
#include <sstream>
#include <boost/assert.hpp>

namespace node
{
	namespace ipt
	{
		//std::ostream& operator<<(std::ostream& os, scramble_key const& v)
		//{
		//	return v.dump(os);
		//}
		
		std::string to_string(scramble_key const& v)
		{
			std::ostringstream ss;
			ss << v;
			return ss.str();
		}

		scramble_key from_string(std::string const& v)
		{
			scramble_key sk;
			std::stringstream ss;
			ss << v;
			ss >> sk;
			return sk;
		}

	}	//	ipt
}	//	node



