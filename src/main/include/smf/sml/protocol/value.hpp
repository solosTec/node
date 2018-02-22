/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_PROTOCOL_VALUE_HPP
#define NODE_SML_PROTOCOL_VALUE_HPP

#include <smf/sml/defs.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/factory/set_factory.h>
#include <type_traits>

namespace node
{
	namespace sml
	{
		namespace detail
		{
			template <typename T>
			struct factory_policy
			{
				//static_assert(false, "not a valid SML data type");
			};

			template<>
			struct factory_policy<bool>
			{
				static cyng::tuple_t create(bool b);
			};
			template<>
			struct factory_policy<std::uint8_t>
			{
				static cyng::tuple_t create(std::uint8_t v);
			};
			template<>
			struct factory_policy<std::uint16_t>
			{
				static cyng::tuple_t create(std::uint16_t v);
			};
			template<>
			struct factory_policy<std::uint32_t>
			{
				static cyng::tuple_t create(std::uint32_t v);
			};
			template<>
			struct factory_policy<std::uint64_t>
			{
				static cyng::tuple_t create(std::uint64_t v);
			};
			template<>
			struct factory_policy<std::int8_t>
			{
				static cyng::tuple_t create(std::int8_t v);
			};
			template<>
			struct factory_policy<std::int16_t>
			{
				static cyng::tuple_t create(std::int16_t v);
			};
			template<>
			struct factory_policy<std::int32_t>
			{
				static cyng::tuple_t create(std::int32_t v);
			};
			template<>
			struct factory_policy<std::int64_t>
			{
				static cyng::tuple_t create(std::int64_t v);
			};
			template<>
			struct factory_policy<std::chrono::system_clock::time_point>
			{
				static cyng::tuple_t create(std::chrono::system_clock::time_point v);
			};
			template<>
			struct factory_policy<cyng::buffer_t>
			{
				static cyng::tuple_t create(cyng::buffer_t&& v);
			};
			template<>
			struct factory_policy<std::string>
			{
				static cyng::tuple_t create(std::string&& v);
			};
			template<>
			struct factory_policy<char*>
			{
				static cyng::tuple_t create(char const* p);
			};
		}

		template <typename T>
		cyng::tuple_t make_value(T&& v)
		{
			using type = typename std::decay<T>::type;
			return detail::factory_policy<type>::create(std::forward<T>(v));
		}

        /**
         * SML Time of type 3 is not defined in SML v1.03
         */
		cyng::tuple_t make_local_timestamp(std::chrono::system_clock::time_point);

		/**
		 * Make a timestamp as type TIME_TIMESTAMP (2)
		 */
		cyng::tuple_t make_timestamp(std::chrono::system_clock::time_point);

		/**
		 * Make a timestamp as type TIME_SECINDEX (1)
		 */
		cyng::tuple_t make_sec_index(std::chrono::system_clock::time_point);

	}
}
#endif
