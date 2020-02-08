/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/intrinsics/obis_factory.hpp>
#include <cyng/factory/factory.hpp>

namespace node
{
	namespace sml
	{

		obis make_obis(std::uint32_t a
			, std::uint32_t b
			, std::uint32_t c
			, std::uint32_t d
			, std::uint32_t e
			, std::uint32_t f)
		{
			return obis(static_cast<std::uint8_t>(a & 0xFF)
				, static_cast<std::uint8_t>(b & 0xFF)
				, static_cast<std::uint8_t>(c & 0xFF)
				, static_cast<std::uint8_t>(d & 0xFF)
				, static_cast<std::uint8_t>(e & 0xFF)
				, static_cast<std::uint8_t>(f & 0xFF));
		}

		cyng::tuple_t tuple_from_path(obis_path path)
		{
			cyng::tuple_t tpl;
			std::transform(path.begin(), path.end(), std::back_inserter(tpl), [](obis const& v) {
				return cyng::make_object(v.to_buffer());
				});
			return tpl;
		}

	}	//	sml
}	//	node

