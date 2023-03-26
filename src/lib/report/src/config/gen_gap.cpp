#include <smf/report/config/gen_gap.h>

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        cyng::tuple_t gen_gap(std::filesystem::path cwd) {
            return cyng::make_tuple(
                create_gap_spec(OBIS_PROFILE_1_MINUTE, cwd, std::chrono::hours(48), false),
                create_gap_spec(OBIS_PROFILE_15_MINUTE, cwd, std::chrono::hours(48), true),
                create_gap_spec(OBIS_PROFILE_60_MINUTE, cwd, std::chrono::hours(800), true),
                create_gap_spec(OBIS_PROFILE_24_HOUR, cwd, std::chrono::hours(800), true));
        }
        cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled) {
            return cyng::make_prop(
                profile,
                cyng::make_tuple(
                    cyng::make_param("name", obis::get_name(profile)),
                    cyng::make_param("path", (cwd / "gap-reports" / sml::get_prefix(profile)).string()),
                    cyng::make_param("backtrack", hours),
                    cyng::make_param("enabled", enabled)));
        }
    } // namespace cfg
} // namespace smf
