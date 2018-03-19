/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/value.hpp>

namespace node
{
	namespace sml
	{
		namespace detail
		{
			cyng::tuple_t factory_policy<bool>::create(bool b)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), b);
			}

			cyng::tuple_t factory_policy<std::uint8_t>::create(std::uint8_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}

			cyng::tuple_t factory_policy<std::uint16_t>::create(std::uint16_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}

			cyng::tuple_t factory_policy<std::uint32_t>::create(std::uint32_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}

			cyng::tuple_t factory_policy<std::uint64_t>::create(std::uint64_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}
			cyng::tuple_t factory_policy<std::int8_t>::create(std::int8_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}
			cyng::tuple_t factory_policy<std::int16_t>::create(std::int16_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}
			cyng::tuple_t  factory_policy<std::int32_t>::create(std::int32_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}
			cyng::tuple_t factory_policy<std::int64_t>::create(std::int64_t v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), v);
			}
			cyng::tuple_t factory_policy<std::chrono::system_clock::time_point>::create(std::chrono::system_clock::time_point v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_TIME), v);
			}
			cyng::tuple_t factory_policy<cyng::buffer_t>::create(cyng::buffer_t&& v)
			{
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), std::forward<cyng::buffer_t>(v));
			}
			cyng::tuple_t factory_policy<std::string>::create(std::string&& v)
			{
				//	convert to buffer_t
				return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_VALUE), cyng::buffer_t(v.begin(), v.end()));
			}
			cyng::tuple_t factory_policy<char*>::create(char const* p)
			{
				//	convert to buffer_t
				return factory_policy<std::string>::create(p);
			}
		}

		cyng::tuple_t make_local_timestamp(std::chrono::system_clock::time_point tp)
		{
			//	1. UNIX timestamp
			std::time_t ut = std::chrono::system_clock::to_time_t(tp);
			//	2. local offset
			//	3. summertime offset

			//	convert to SML_Time type 3
			//	SML_Time is a tuple with 2 elements
			//	(1) choice := 2 (timestamp)
			//	(2) value := UNIX time
			//	(3) value := UNIX time + offset data
			return cyng::tuple_factory(static_cast<std::uint8_t>(PROC_PAR_TUPELENTRY)
				, cyng::tuple_factory(static_cast<std::uint32_t>(ut)
					, static_cast<std::uint8_t>(0)		//	local offset
					, static_cast<std::uint8_t>(0)));	//	summertime offset
		}

		cyng::tuple_t make_timestamp(std::chrono::system_clock::time_point tp)
		{
			//	1. UNIX timestamp - Y2K38 problem.
			std::time_t ut = std::chrono::system_clock::to_time_t(tp);
			return cyng::tuple_factory(static_cast<std::uint8_t>(TIME_TIMESTAMP), static_cast<std::uint32_t>(ut));
		}

		cyng::tuple_t make_sec_index(std::chrono::system_clock::time_point tp)
		{
			//	1. UNIX timestamp - Y2K38 problem.
			std::time_t ut = std::chrono::system_clock::to_time_t(tp);
			return cyng::tuple_factory(static_cast<std::uint8_t>(TIME_SECINDEX), static_cast<std::uint32_t>(ut));
		}

	}
}