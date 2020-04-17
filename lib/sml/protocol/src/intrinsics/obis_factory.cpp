/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/obis_io.h>

#include <cyng/factory/factory.hpp>
#include <cyng/factory/set_factory.h>
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
			C C_from_path(obis_path_t path)
			{
				C container;
				std::transform(path.begin(), path.end(), std::back_inserter(container), [](obis const& v) {
					return cyng::make_object(v.to_buffer());
					});
				return container;
			}

			template<typename C>
			obis_path_t C_to_path(C container)
			{
				obis_path_t path;
				std::transform(container.begin(), container.end(), std::back_inserter(path), [](cyng::object obj) {
					return cyng::to_buffer(obj);
					});
				return path;
			}
		}

		cyng::tuple_t tuple_from_path(obis_path_t path)
		{
			return C_from_path<cyng::tuple_t>(path);
		}

		cyng::vector_t vector_from_path(obis_path_t path)
		{
			return C_from_path<cyng::vector_t>(path);
		}

		obis_path_t tuple_to_path(cyng::tuple_t tpl)
		{
			return C_to_path<cyng::tuple_t>(tpl);
		}

		obis_path_t vector_to_path(cyng::vector_t vec)
		{
			return C_to_path<cyng::vector_t>(vec);
		}

		obis_path_t vector_to_path(std::vector<std::string> const& vec)
		{
			obis_path_t path;
			std::transform(vec.begin(), vec.end(), std::back_inserter(path), [](std::string const& s) {
				auto const r = parse_obis(s);
				return (r.second)
					? r.first
					: obis();
				});
			return path;
		}

		cyng::vector_t path_to_vector(obis_path_t path)
		{
			std::vector<std::string> vec;
			std::transform(path.begin(), path.end(), std::back_inserter(vec), [](obis const& code) {
				return code.to_str();
				});
			return cyng::vector_factory<std::string>(vec);
		}

	}	//	sml
}	//	node

