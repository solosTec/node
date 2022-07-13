#include <smf/obis/profile.h>

#include <smf/obis/defs.h>

//#ifdef _DEBUG
//#include <cyng/io/ostream.h>
//#include <cyng/io/serialize.h>
//#include <iostream>
//#endif

namespace smf {
    namespace sml {
        cyng::obis_path_t get_profiles() {
            return {
                OBIS_PROFILE_1_MINUTE,
                OBIS_PROFILE_15_MINUTE,
                OBIS_PROFILE_60_MINUTE,
                OBIS_PROFILE_24_HOUR,
                // OBIS_PROFILE_LAST_2_HOURS,
                // OBIS_PROFILE_LAST_WEEK,
                OBIS_PROFILE_1_MONTH,
                OBIS_PROFILE_1_YEAR,
                // OBIS_PROFILE_INITIAL
            };
        }

        bool is_profile(cyng::obis code) {
            auto const profiles = get_profiles();
            return std::find(profiles.begin(), profiles.end(), code) != profiles.end();
        }

        std::chrono::seconds interval_time(cyng::obis profile) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(1));
            case CODE_PROFILE_15_MINUTE: return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(15));
            case CODE_PROFILE_60_MINUTE: return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(1));
            case CODE_PROFILE_24_HOUR:
#if __cplusplus >= 202002L
                return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days(1));
#else
                return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(24));
#endif
            case CODE_PROFILE_1_MONTH:
#if __cplusplus >= 202002L
                return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days(30));
#else
                return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(24 * 30));
#endif
            case CODE_PROFILE_1_YEAR:
#if __cplusplus >= 202002L
                return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days(365));
#else
                return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(24 * 30 * 365));
#endif
            default: BOOST_ASSERT_MSG(false, "not implemented yet"); break;
            }

            //
            //  this is an error
            //
            BOOST_ASSERT_MSG(false, "not a load profile");
            return std ::chrono::minutes(1);
        }

        std::int64_t rasterize_interval(std::int64_t sec, cyng::obis profile) {
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

            case CODE_PROFILE_1_MONTH: {
                // auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
                //   assume 30 days
                if (sec < 86400u * 30u) {
                    sec = 86400u * 30u;
                }
                //	rasterization to full days
                sec -= (sec % 86400u);
                break;
            }
            case CODE_PROFILE_1_YEAR: {
                //   assume 364 days
                if (sec < 86400u * 364u) {
                    sec = 86400u * 364u;
                }
                //	rasterization to full days
                sec -= (sec % 86400u);
                break;
            }
            default:
                //
                //  ToDo: implement
                //
                BOOST_ASSERT_MSG(false, "not implemented yet");
                break;
            }
            return sec;
        }

        std::chrono::system_clock::time_point
        next(std::chrono::seconds interval, cyng::obis profile, std::chrono::system_clock::time_point now) {

            //
            //  clean up parameters.
            //  count is interval in seconds
            //
            auto count = rasterize_interval(interval.count(), profile);
            BOOST_ASSERT(count > 0);
            auto const epoch = std::chrono::time_point<std::chrono::system_clock>{};

            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: {
                auto const minutes = minutes_since_epoch(now).count();
                return epoch + std::chrono::minutes(minutes) + std::chrono::minutes(count / 60u);
            } break;

            case CODE_PROFILE_15_MINUTE: {
                auto const quarters = minutes_since_epoch(now).count() / 15u;
                return epoch + std::chrono::minutes(quarters * 15u) + std::chrono::minutes(count / 60u);
            } break;

            case CODE_PROFILE_60_MINUTE: {
                auto const hours = hours_since_epoch(now).count();
                return epoch + std::chrono::hours(hours) + std::chrono::hours(count / 3600u);
            } break;

            case CODE_PROFILE_24_HOUR: {
                auto const days = hours_since_epoch(now).count() / 24;
                return epoch + std::chrono::hours(days * 24) + std::chrono::hours(count / 3600u * 24u);
            } break;

            case CODE_PROFILE_1_MONTH:
                //  ToDo: use a calendar to get the length of the months
                return now + std::chrono::hours(24u * 30u);

            case CODE_PROFILE_1_YEAR:
                //  ToDo: use a calendar to get the length of the year
                return now + std::chrono::hours(24u * 365u);

            default: break;
            }

            return now;
        }

        std::int64_t to_index(std::chrono::system_clock::time_point now, cyng::obis profile) {

            BOOST_ASSERT(now > offset_);

            auto const start = calculate_offset(now);
            //#ifdef _DEBUG
            //            std::cout << "start: " << start << std::endl;
            //#endif

            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return minutes_since_epoch(start).count();
            case CODE_PROFILE_15_MINUTE: return minutes_since_epoch(start).count() / 15u;
            case CODE_PROFILE_60_MINUTE: return hours_since_epoch(start).count();
            case CODE_PROFILE_24_HOUR: return hours_since_epoch(start).count() / 24u;
            case CODE_PROFILE_1_MONTH:
                //  ToDo: calculate the exact count of months
                return hours_since_epoch(start).count() / (24u * 30u);
            case CODE_PROFILE_1_YEAR:
                //  ToDo: calculate the exact count of years
                return hours_since_epoch(start).count() / (24u * 364u);
            default: break;
            }

            return std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count();
        }

        std::chrono::system_clock::time_point to_time_point(std::int64_t idx, cyng::obis profile) {

            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return offset_ + std::chrono::minutes(idx);
            case CODE_PROFILE_15_MINUTE: return offset_ + std::chrono::minutes(idx * 15u);
            case CODE_PROFILE_60_MINUTE: return offset_ + std::chrono::hours(idx);
            case CODE_PROFILE_24_HOUR: return offset_ + std::chrono::hours(idx * 24u);
            case CODE_PROFILE_1_MONTH:
                //  ToDo: calculate the exact count of months
                return offset_ + std::chrono::hours(idx * 24u * 30u);
            case CODE_PROFILE_1_YEAR:
                //  ToDo: calculate the exact count of years
                return offset_ + std::chrono::hours(idx * 24u * 30u * 364u);
            default: break;
            }

            return offset_ + std::chrono::minutes(idx);
        }

    } // namespace sml

    std::chrono::minutes minutes_since_epoch(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::minutes>(tp.time_since_epoch());
    }

    std::chrono::hours hours_since_epoch(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::hours>(tp.time_since_epoch());
    }

    std::chrono::system_clock::time_point calculate_offset(std::chrono::system_clock::time_point tp) {
        auto const epoch = std::chrono::time_point<std::chrono::system_clock>{};
        return tp - (offset_ - epoch);
    }
} // namespace smf
