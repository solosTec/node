#include <smf/obis/profile.h>

#include <smf/obis/defs.h>

#include <cyng/obj/intrinsics/date.h>

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

        std::chrono::minutes interval_time(std::chrono::system_clock::time_point tp, cyng::obis profile) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return std::chrono::minutes(1);
            case CODE_PROFILE_15_MINUTE: return std::chrono::minutes(15);
            case CODE_PROFILE_60_MINUTE: return std::chrono::hours(1);
            case CODE_PROFILE_24_HOUR: return std::chrono::hours(24);
            case CODE_PROFILE_1_MONTH: return std::chrono::hours(cyng::date::make_date_from_local_time(tp).days_in_month() * 24);
            case CODE_PROFILE_1_YEAR: return std::chrono::hours(cyng::date::make_date_from_local_time(tp).days_in_year() * 24);
            default: BOOST_ASSERT_MSG(false, "not implemented yet"); break;
            }

            //
            //  this is an error
            //
            BOOST_ASSERT_MSG(false, "not a load profile");
            return std::chrono::minutes(1);
        }

        std::chrono::hours backtrack_time(cyng::obis profile) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return std::chrono::hours(2); ;
            case CODE_PROFILE_15_MINUTE: return std::chrono::hours(48);
            case CODE_PROFILE_60_MINUTE: return std::chrono::hours(52);
            case CODE_PROFILE_24_HOUR: return std::chrono::hours(60);
            case CODE_PROFILE_1_MONTH: return std::chrono::hours(24 * 40);
            case CODE_PROFILE_1_YEAR: return std::chrono::hours(24 * 400);
            default: break;
            }
            //  error
            BOOST_ASSERT_MSG(false, "not implemented yet");
            return std::chrono::hours(48);
        }

        std::chrono::system_clock::time_point floor(std::chrono::system_clock::time_point tp, cyng::obis profile) {
            auto const epoch = std::chrono::time_point<std::chrono::system_clock>{};

            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return epoch + minutes_since_epoch(tp);
            case CODE_PROFILE_15_MINUTE: return epoch + (minutes_since_epoch(tp) / 15) * 15;
            case CODE_PROFILE_60_MINUTE: return epoch + hours_since_epoch(tp);
            case CODE_PROFILE_24_HOUR: return epoch + (hours_since_epoch(tp) / 24) * 24;
            case CODE_PROFILE_1_MONTH: {
                auto const days = cyng::date::make_date_from_local_time(tp).days_in_month();
                return epoch + (hours_since_epoch(tp) / days) * days;
            }
            case CODE_PROFILE_1_YEAR: {
                auto const days = cyng::date::make_date_from_local_time(tp).days_in_year();
                return epoch + (hours_since_epoch(tp) / days) * days;
            }
            default: break;
            }
            //  error
            BOOST_ASSERT_MSG(false, "not implemented yet");
            return tp;
        }

        std::chrono::system_clock::time_point round(std::chrono::system_clock::time_point tp, cyng::obis profile) {
            auto const epoch = std::chrono::time_point<std::chrono::system_clock>{};
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return floor(tp + std::chrono::seconds(30), profile);
            case CODE_PROFILE_15_MINUTE: return floor(tp + std::chrono::minutes(7), profile);
            case CODE_PROFILE_60_MINUTE: return floor(tp + std::chrono::minutes(30), profile);
            case CODE_PROFILE_24_HOUR: return floor(tp + std::chrono::hours(12), profile);
            case CODE_PROFILE_1_MONTH: return floor(tp + std::chrono::hours(24), profile);
            case CODE_PROFILE_1_YEAR: return floor(tp + std::chrono::hours(24), profile);
            default: break;
            }
            //  error
            BOOST_ASSERT_MSG(false, "not implemented yet");
            return tp;
        }

        std::int64_t rasterize_interval(std::int64_t sec, cyng::obis profile) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_INITIAL:
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

            case CODE_PROFILE_1_MONTH: {
                //  use a calendar to get the length of the months
                auto const d = cyng::date::make_date_from_local_time(now).get_end_of_month();
                return d.to_utc_time_point() + std::chrono::hours(24);
            }

            case CODE_PROFILE_1_YEAR: {
                //  use a calendar to get the length of the year
                auto const d = cyng::date::make_date_from_local_time(now).get_end_of_year();
                return d.to_utc_time_point() + std::chrono::hours(24);
            }

            default: break;
            }

            return now;
        }

        std::pair<std::int64_t, bool> to_index(std::chrono::system_clock::time_point now, cyng::obis profile) {

            // BOOST_ASSERT(now > offset_);
            if (now < offset_) {
                //  invalid time point
                return {0, false};
            }

            auto const start = calculate_offset(now);
            // #ifdef _DEBUG
            //             std::cout << "start: " << start << std::endl;
            // #endif

            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return {minutes_since_epoch(start).count(), true};
            case CODE_PROFILE_15_MINUTE: return {minutes_since_epoch(start).count() / 15u, true};
            case CODE_PROFILE_60_MINUTE: return {hours_since_epoch(start).count(), true};
            case CODE_PROFILE_24_HOUR: return {hours_since_epoch(start).count() / 24u, true};
            case CODE_PROFILE_1_MONTH: {
                //  calculate the exact count of months
                auto const days = cyng::date::make_date_from_local_time(now).days_in_month();
                return {hours_since_epoch(start).count() / days, true};
            }
            case CODE_PROFILE_1_YEAR: {
                //  calculate the exact count of years
                auto const days = cyng::date::make_date_from_local_time(now).days_in_year();
                return {hours_since_epoch(start).count() / days, true};
            }
            default: break;
            }

            //  unknown profile
            return {std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count(), false};
        }

        std::pair<std::int64_t, bool> to_index(cyng::date ts, cyng::obis profile) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return {ts.calculate_slot(std::chrono::minutes(1)), true};
            case CODE_PROFILE_15_MINUTE: return {ts.add(std::chrono::minutes(5)).calculate_slot(std::chrono::minutes(15)), true};
            case CODE_PROFILE_60_MINUTE: return {ts.add(std::chrono::minutes(15)).calculate_slot(std::chrono::hours(1)), true};
            case CODE_PROFILE_24_HOUR: return {ts.add(std::chrono::hours(1)).calculate_slot(std::chrono::hours(24)), true};
            case CODE_PROFILE_1_MONTH: break;
            case CODE_PROFILE_1_YEAR: break;
            default: break;
            }
            return {ts.calculate_slot(std::chrono::minutes(15)), false};
        }

        std::pair<std::int64_t, std::int64_t> to_index_range(cyng::date ts, cyng::obis profile) {
            //
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: break;
            case CODE_PROFILE_15_MINUTE: break;
            case CODE_PROFILE_60_MINUTE: break;
            case CODE_PROFILE_24_HOUR:
                //  full month
                break;
            case CODE_PROFILE_1_MONTH:
                //  full year
                break;
            case CODE_PROFILE_1_YEAR: break;
            default: break;
            }
            //  full day
            return {to_index(ts.get_start_of_day(), profile).first, to_index(ts.get_end_of_day(), profile).first};
        }

        cyng::date from_index_to_date(std::int64_t idx, cyng::obis profile) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return cyng::make_epoch_date() + std::chrono::minutes(idx);
            case CODE_PROFILE_15_MINUTE: return cyng::make_epoch_date() + std::chrono::minutes(idx * 15);
            case CODE_PROFILE_60_MINUTE: return cyng::make_epoch_date() + std::chrono::hours(idx);
            case CODE_PROFILE_24_HOUR: return cyng::make_epoch_date() + std::chrono::hours(idx * 24);
            case CODE_PROFILE_1_MONTH: break;
            case CODE_PROFILE_1_YEAR: break;
            default: break;
            }
            return cyng::make_epoch_date();
        }

        std::size_t calculate_entry_count(cyng::obis profile, std::chrono::hours span) {
            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE: return (span.count() * 60u);
            case CODE_PROFILE_15_MINUTE: return (span.count() * 60u) / 15u;
            case CODE_PROFILE_60_MINUTE: return span.count();
            case CODE_PROFILE_24_HOUR: return span.count() / 24u;
            case CODE_PROFILE_1_MONTH:
            case CODE_PROFILE_1_YEAR:
            default: break;
            }
            return 0;
        }

        std::chrono::system_clock::time_point restore_time_point(std::int64_t idx, cyng::obis profile) {

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

        cyng::date get_offset() { return cyng::date::make_date_from_utc_time(offset_); }

    } // namespace sml

    std::chrono::minutes minutes_since_epoch(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::minutes>(tp.time_since_epoch());
    }

    std::chrono::hours hours_since_epoch(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::hours>(tp.time_since_epoch());
    }

    std::chrono::system_clock::time_point calculate_offset(std::chrono::system_clock::time_point tp) {
        auto const epoch = std::chrono::time_point<std::chrono::system_clock>{};
        return tp - (sml::offset_ - epoch);
    }
} // namespace smf
