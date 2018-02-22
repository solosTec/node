/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_SCALER_H
#define NODE_LIB_SML_SCALER_H

#include <string>
#include <cstdint>

namespace node
{
	namespace sml
	{
		std::string scale_value(std::int64_t value, std::int8_t scaler);
	}
}
#endif