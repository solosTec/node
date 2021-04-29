/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2032 Sylko Olzscher
 *
 */

//#include <smf/sml/serializer.h>
#include <cyng/io/io.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/eod.h>

#include <chrono>

namespace cyng {
	namespace io {

		/**
		 * Serialization to SML.
		 */
		struct SML {};


		template <typename T>
		struct serializer<T, SML>
		{
			static std::size_t write(std::ostream& os, T const& v)
			{
				auto const pos = os.tellp();
				//	optional
				os.put(0x01);	//	TL field without data
				return os.tellp() - pos;
			}
		};

		template <>
		struct serializer <eod, SML>
		{
			static std::size_t write(std::ostream& os, cyng::eod v);
		};

		template <>
		struct serializer <bool, SML>
		{
			static std::size_t write(std::ostream& os, bool v);
		};

		template <>
		struct serializer <std::uint8_t, SML>
		{
			static std::size_t write(std::ostream& os, std::uint8_t v);
		};

		template <>
		struct serializer <std::uint16_t, SML>
		{
			static std::size_t write(std::ostream& os, std::uint16_t v);
		};

		template <>
		struct serializer <std::uint32_t, SML>
		{
			static std::size_t write(std::ostream& os, std::uint32_t v);
		};

		template <>
		struct serializer <std::uint64_t, SML>
		{
			static std::size_t write(std::ostream& os, std::uint64_t v);
		};

		template <>
		struct serializer <std::int8_t, SML>
		{
			static std::size_t write(std::ostream& os, std::int8_t v);
		};

		template <>
		struct serializer <std::int16_t, SML>
		{
			static std::size_t write(std::ostream& os, std::int16_t v);
		};

		template <>
		struct serializer <std::int32_t, SML>
		{
			static std::size_t write(std::ostream& os, std::int32_t v);
		};

		template <>
		struct serializer <std::int64_t, SML>
		{
			static std::size_t write(std::ostream& os, std::int64_t v);
		};

		template <>
		struct serializer <cyng::buffer_t, SML>
		{
			static std::size_t write(std::ostream& os, cyng::buffer_t const& v);
		};

		template <>
		struct serializer <std::string, SML>
		{
			static std::size_t write(std::ostream& os, std::string const& v);
		};

		template <>
		struct serializer <std::chrono::system_clock::time_point, SML>
		{
			static std::size_t write(std::ostream& os, std::chrono::system_clock::time_point v);
		};

		template <>
		struct serializer <cyng::tuple_t, SML>
		{
			static std::size_t write(std::ostream& os, cyng::tuple_t const& v);
		};

#ifdef NODE_UNIT_TEST
		void write_length_field(std::ostream& os, std::uint32_t length, std::uint8_t type);
#endif

	}
}
