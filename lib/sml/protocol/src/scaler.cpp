/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/scaler.h>
#include <boost/assert.hpp>

namespace node
{
	namespace sml
	{
		std::string scale_value(std::int64_t value, std::int8_t scaler)
		{
			//	Working with a string operation prevents us of losing precision.
			std::string str_value = std::to_string(value);

			//	treat 0 as special value
			if (value == 0)	return str_value;

			//	should at least contain a "0"
			BOOST_ASSERT_MSG(!str_value.empty(), "no number");
			if (str_value.empty())	return "0.0";	//	emergency exit

			if (scaler < 0)
			{

				//	negative
				while ((std::size_t)std::abs(scaler - 1) > str_value.size())
				{
					//	pad missing zeros
					str_value.insert(0, "0");
				}

				//	finish
				if (str_value.size() > (std::size_t)std::abs(scaler))
				{
					const std::size_t pos = str_value.size() + scaler;
					str_value.insert(pos, ".");
				}
			}
			else
			{
				//	simulating: result = value * pow10(scaler)
				while (scaler-- > 0)
				{
					//	append missing zeros
					str_value.append("0");
				}
			}

			return str_value;
		}
	}
}
