/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/intrinsics/obis_factory.hpp>
#include <cyng/factory/factory.hpp>
#include <cyng/buffer_cast.h>

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

		namespace {
			template<typename C>
			C C_from_path(obis_path path)
			{
				C container;
				std::transform(path.begin(), path.end(), std::back_inserter(container), [](obis const& v) {
					return cyng::make_object(v.to_buffer());
					});
				return container;
			}

			template<typename C>
			obis_path C_to_path(C container)
			{
				obis_path path;
				std::transform(container.begin(), container.end(), std::back_inserter(path), [](cyng::object obj) {
					return cyng::to_buffer(obj);
					});
				return path;
			}
		}

		cyng::tuple_t tuple_from_path(obis_path path)
		{
			return C_from_path<cyng::tuple_t>(path);
		}

		cyng::vector_t vector_from_path(obis_path path)
		{
			return C_from_path<cyng::vector_t>(path);
		}

		obis_path tuple_to_path(cyng::tuple_t tpl)
		{
			return C_to_path<cyng::tuple_t>(tpl);
		}

		obis_path vector_to_path(cyng::vector_t vec)
		{
			return C_to_path<cyng::vector_t>(vec);
		}

	}	//	sml
}	//	node

