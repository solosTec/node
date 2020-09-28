/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>

#include <cyng/factory/factory.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/value_cast.hpp>
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

		obis make_obis(obis const& code, std::uint32_t f)
		{
			return obis(code.get_medium()
				, code.get_channel()
				, code.get_indicator()
				, code.get_mode()
				, code.get_quantities()
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

		obis_path_t vector_to_path(cyng::vector_t const& vec)
		{
			obis_path_t path;
			std::transform(vec.begin(), vec.end(), std::back_inserter(path), [](cyng::object obj) {

				switch (obj.get_class().tag()) {
				case cyng::TC_BUFFER:
					return obis(cyng::to_buffer(obj));
				case cyng::TC_STRING:
				{
					auto const str = cyng::value_cast<std::string>(obj, "");
					auto const r = parse_obis(str);
					return (r.second)
						? r.first
						: make_obis(0, 0, 0, 0, 0, 0);
				}
				case 267u:	//	PREDEF_CUSTOM_02
					return cyng::value_cast(obj, obis());
				default:
					break;
				}
				return make_obis(0, 0, 0, 0, 0, 0);
			});
			BOOST_ASSERT(vec.size() == path.size());
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

		cyng::vector_t transform_to_obj_vector(obis_path_t const& path, bool translate)
		{
			auto const sv = transform_to_str_vector(path, translate);
			cyng::vector_t vec;
			std::transform(sv.begin(), sv.end(), std::back_inserter(vec), [](std::string code) {
				return cyng::make_object(code);
				});
			return vec;
		}

		std::vector<std::string> transform_to_str_vector(obis_path_t const& path, bool translate)
		{
			std::vector<std::string> vec;
			std::transform(path.begin(), path.end(), std::back_inserter(vec), [&](obis code) {
				return (translate)
					? get_name(code)
					: code.to_str();
				});
			return vec;
		}

	}	//	sml
}	//	node

