/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/serializer.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/eod.h>
#include <chrono>

namespace node
{
	namespace sml
	{
		template <typename T>
		struct serializer
		{
			static std::ostream& write(std::ostream& os, T const& v)
			{
				//	optional
				os.put(0x01);	//	TL field without data
				return os;
			}
		};

		template <>
		struct serializer <cyng::eod>
		{
			static std::ostream& write(std::ostream& os, cyng::eod v);
		};

		template <>
		struct serializer <bool>
		{
			static std::ostream& write(std::ostream& os, bool v);
		};

		template <>
		struct serializer <std::uint8_t>
		{
			static std::ostream& write(std::ostream& os, std::uint8_t v);
		};

		template <>
		struct serializer <std::uint16_t>
		{
			static std::ostream& write(std::ostream& os, std::uint16_t v);
		};

		template <>
		struct serializer <std::uint32_t>
		{
			static std::ostream& write(std::ostream& os, std::uint32_t v);
		};

		template <>
		struct serializer <std::uint64_t>
		{
			static std::ostream& write(std::ostream& os, std::uint64_t v);
		};

		template <>
		struct serializer <std::int8_t>
		{
			static std::ostream& write(std::ostream& os, std::int8_t v);
		};

		template <>
		struct serializer <std::int16_t>
		{
			static std::ostream& write(std::ostream& os, std::int16_t v);
		};

		template <>
		struct serializer <std::int32_t>
		{
			static std::ostream& write(std::ostream& os, std::int32_t v);
		};

		template <>
		struct serializer <std::int64_t>
		{
			static std::ostream& write(std::ostream& os, std::int64_t v);
		};

		template <>
		struct serializer <cyng::buffer_t>
		{
			static std::ostream& write(std::ostream& os, cyng::buffer_t const& v);
		};

		template <>
		struct serializer <std::string>
		{
			static std::ostream& write(std::ostream& os, std::string const& v);
		};

		template <>
		struct serializer <std::chrono::system_clock::time_point>
		{
			static std::ostream& write(std::ostream& os, std::chrono::system_clock::time_point v);
		};

		template <>
		struct serializer <cyng::tuple_t>
		{
			static std::ostream& write(std::ostream& os, cyng::tuple_t const& v);
		};
	}
}
