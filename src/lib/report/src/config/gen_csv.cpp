#include <smf/report/config/gen_csv.h>

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        cyng::tuple_t gen_csv(std::filesystem::path cwd) {
            return cyng::make_tuple(
                cyng::make_param("db", "default"),
                cyng::make_param(
                    "profiles",
                    cyng::make_tuple(
                        create_csv_spec(OBIS_PROFILE_1_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MINUTE)),
                        create_csv_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                        create_csv_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                        create_csv_spec(OBIS_PROFILE_24_HOUR, cwd, true, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                        // create_report_spec(OBIS_PROFILE_LAST_2_HOURS, cwd, false,
                        // sml::backtrack_time(OBIS_PROFILE_LAST_2_HOURS)), create_report_spec(OBIS_PROFILE_LAST_WEEK, cwd,
                        // false, sml::backtrack_time(OBIS_PROFILE_LAST_WEEK)),
                        create_csv_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH)), // one month
                        create_csv_spec(OBIS_PROFILE_1_YEAR, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_YEAR))    //  one year
                        ) // reports tuple
                    ));
        }
        cyng::prop_t create_csv_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
            return cyng::make_prop(
                profile,
                cyng::make_tuple(
                    cyng::make_param("name", obis::get_name(profile)),
                    cyng::make_param("path", (cwd / "csv.reports").string()),
                    cyng::make_param("backtrack", backtrack),
                    cyng::make_param("prefix", sml::get_prefix(profile)),
                    cyng::make_param("enabled", enabled)));
        }
    } // namespace cfg
} // namespace smf
