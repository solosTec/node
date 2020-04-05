/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "profiles.h"
#include <smf/sml/obis_db.h>

#include <cyng/chrono.h>

namespace node
{
	namespace sml
	{

		std::uint32_t rasterize_interval(std::uint32_t sec, obis profile)
		{
			switch (profile.to_uint64()) {
			case CODE_PROFILE_1_MINUTE:
				//	guarantee that interval has at least 1 minute
				if (sec < 60u) {
					sec = 60u;
				}
				//	rasterization to full minutes
				sec -= (sec % 60u);
				break;

			case CODE_PROFILE_15_MINUTE:
				//	guarantee that interval has at least 15 minutes
				if (sec < 900u) {
					sec = 900u;
				}

				//	rasterization to quarter hour
				sec -= (sec % 900);
				break;

			case CODE_PROFILE_60_MINUTE:
				//	guarantee that interval has at least 60 minutes
				if (sec < 3600u) {
					sec = 3600u;
				}

				//	rasterization to full hours
				sec -= (sec % 3600u);
				break;

			case CODE_PROFILE_24_HOUR:
				//	guarantee that interval has at least 24 hours
				if (sec < 86400u) {
					sec = 86400u;
				}

				//	rasterization to full hours
				sec -= (sec % 3600u);
				break;

			case CODE_PROFILE_1_MONTH:
			case CODE_PROFILE_1_YEAR:
			default:
				break;
			}
			return sec;
		}

		std::chrono::system_clock::time_point next(std::chrono::seconds interval, obis profile)
		{
			auto count = interval.count();
			auto const now = std::chrono::system_clock::now();

			switch (profile.to_uint64()) {
			case CODE_PROFILE_1_MINUTE:
			{
				auto const minutes = cyng::chrono::minutes_since_epoch(now);
				return cyng::chrono::ts_since_passed_minutes(minutes + (count / 60u));
			}
				break;

			case CODE_PROFILE_15_MINUTE:
			{
				auto const quarters = cyng::chrono::minutes_since_epoch(now) / 15u;
				return cyng::chrono::ts_since_passed_minutes((quarters * 15u) + (count / 60u));
			}
				break;

			case CODE_PROFILE_60_MINUTE:
			{
				auto const hours = cyng::chrono::hours_since_epoch(now);
				return cyng::chrono::ts_since_passed_hours(hours + (count / 3600u));

			}
				break;

			case CODE_PROFILE_24_HOUR:
			{
				auto const hours = cyng::chrono::hours_since_epoch(now);
				return cyng::chrono::ts_since_passed_hours(hours + (count / 3600u));
			}
				break;

			case CODE_PROFILE_1_MONTH:
				return now + std::chrono::hours(24u * 30u);

			case CODE_PROFILE_1_YEAR:
				return now + std::chrono::hours(24u * 365u);

			default:
				break;
			}

			return now;
		}

	}
}

