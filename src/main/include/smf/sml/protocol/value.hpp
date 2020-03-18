/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_PROTOCOL_VALUE_HPP
#define NODE_SML_PROTOCOL_VALUE_HPP

#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
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

			/**
			 * Catch all <const T&>  
			 */
			template<typename T>
			struct factory_policy<const T&>
			{
				using factory = factory_policy<T>;
				static cyng::tuple_t create(T v)
				{
					return factory::create(v);
				}
			};

			/**
			 * Catch all <T&>  
			 * make a copy => && rvalue
			 */
			template<typename T>
			struct factory_policy<T&>
			{
				using factory = factory_policy<T>;
				static cyng::tuple_t create(T& v)
				{
					//
					//	make a copy => && rvalue
					//
					return factory::create(T(v));
				}
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
			struct factory_policy<std::chrono::milliseconds>
			{
				static cyng::tuple_t create(std::chrono::milliseconds v);
			};
			template<>
			struct factory_policy<std::chrono::seconds>
			{
				static cyng::tuple_t create(std::chrono::seconds v);
			};
			template<>
			struct factory_policy<cyng::buffer_t>
			{
				static cyng::tuple_t create(cyng::buffer_t&& v);
			};
			template<>
			struct factory_policy<const cyng::buffer_t&>
			{
				static cyng::tuple_t create(cyng::buffer_t const& v);
			};
			template<>
			struct factory_policy<cyng::buffer_t&>
			{
				static cyng::tuple_t create(cyng::buffer_t& v);
			};
			template<>
			struct factory_policy<std::string>
			{
				static cyng::tuple_t create(std::string&& v);
			};
			template<>
			struct factory_policy<const std::string&>
			{
				static cyng::tuple_t create(const std::string&);
			};

			template< std::size_t N >
			struct factory_policy<const char(&)[N]>
			{
				static cyng::tuple_t create(const char (&p)[N])
				{
					return factory_policy<std::string>::create(std::string(p, N - 1));
				}
		 	};

			template<>
			struct factory_policy<char*>
			{
				static cyng::tuple_t create(char const* p);
			};

			template<>
			struct factory_policy<obis>
			{
				static cyng::tuple_t create(obis&& v);
			};
			template<>
			struct factory_policy<const obis&>
			{
				static cyng::tuple_t create(obis const& v);
			};
			template<>
			struct factory_policy<boost::asio::ip::address>
			{
				static cyng::tuple_t create(boost::asio::ip::address v);
			};

			template<>
			struct factory_policy<cyng::crypto::aes_128_key>
			{
				static cyng::tuple_t create(cyng::crypto::aes_128_key v);
			};
			template<>
			struct factory_policy<cyng::crypto::aes_192_key>
			{
				static cyng::tuple_t create(cyng::crypto::aes_192_key v);
			};
			template<>
			struct factory_policy<cyng::crypto::aes_256_key>
			{
				static cyng::tuple_t create(cyng::crypto::aes_256_key v);
			};

			template<>
			struct factory_policy<cyng::object>
			{
				static cyng::tuple_t create(cyng::object const& v);
			};
		}

		template <typename T>
		cyng::tuple_t make_value(T&& v)
		{
			//
			//	When using decay<T> to get the plain data type we cannot longer
			//	detect string literals. Without decay<T> we have to handle each 
			//	variation with const and & individually.
			//

			//using type = typename std::decay<T>::type;
			//return detail::factory_policy<type>::create(std::forward<T>(v));
			return detail::factory_policy<T>::create(std::forward<T>(v));
		}

		/**
		 * generate an empty value
		 */
		cyng::tuple_t make_value();

		
		/**
         * SML Time of type 3 is used but not defined in SML v1.03
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
		cyng::tuple_t make_sec_index_value(std::chrono::system_clock::time_point);

	}
}
#endif
