#include <profiles.h>

#include <smf/obis/defs.h>

namespace smf {

    std::uint32_t rasterize_interval(std::uint32_t sec, cyng::obis profile) {
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
            //
            //  ToDo: implement
            //
            BOOST_ASSERT_MSG(false, "not implemented yet");
            break;
        }
        return sec;
    }

    std::chrono::system_clock::time_point next(std::chrono::seconds interval, cyng::obis profile) {
        auto count = interval.count();
        auto const now = std::chrono::system_clock::now();
        auto const epoch = std::chrono::time_point<std::chrono::system_clock>{};

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: {
            auto const minutes = minutes_since_epoch(now).count();
            return epoch + std::chrono::minutes(minutes + (count / 60u));
        } break;

        case CODE_PROFILE_15_MINUTE: {
            auto const quarters = minutes_since_epoch(now).count() / 15u;
            return epoch + std::chrono::minutes((quarters * 15u) + (count / 60u));
        } break;

        case CODE_PROFILE_60_MINUTE: {
            auto const hours = hours_since_epoch(now).count();
            return epoch + std::chrono::hours(hours + (count / 3600u));
        } break;

        case CODE_PROFILE_24_HOUR: {
            auto const hours = hours_since_epoch(now).count();
            return epoch + std::chrono::hours(hours + (count / 3600u));
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

    std::chrono::minutes minutes_since_epoch(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::minutes>(tp.time_since_epoch());
    }

    std::chrono::hours hours_since_epoch(std::chrono::system_clock::time_point tp) {
        return std::chrono::duration_cast<std::chrono::hours>(tp.time_since_epoch());
    }

} // namespace smf
