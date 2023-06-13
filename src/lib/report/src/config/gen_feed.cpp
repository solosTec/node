#include <smf/report/config/gen_feed.h>

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/container_factory.hpp>

#include <boost/assert.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        cyng::tuple_t gen_feed(std::filesystem::path cwd) {
            return cyng::make_tuple(
                cyng::make_param("db", "default"),
                cyng::make_param("print.version", true), // if true first line contains the LPEx version
                cyng::make_param(
                    "debug",
#ifdef _DEBUG
                    true
#else
                    false
#endif
                    ), // if true the generated LPex files contain debug data
                cyng::make_param(
                    "profiles",
                    cyng::make_tuple(
                        create_feed_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                        create_feed_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                        create_feed_spec(OBIS_PROFILE_24_HOUR, cwd, false, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                        create_feed_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH))) // feed tuple
                    )                                                                                                  // feed param
            );
        }

        cyng::prop_t create_feed_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
            return cyng::make_prop(
                profile,
                cyng::make_tuple(
                    cyng::make_param("name", obis::get_name(profile)),
                    cyng::make_param("path", (cwd / "feed.reports").string()),
                    cyng::make_param("backtrack", backtrack),
                    cyng::make_param("prefix", sml::get_prefix(profile)),
                    cyng::make_param("add.customer.data", false), // add/update customer data
                    cyng::make_param("enabled", enabled),
                    cyng::make_param("shift.factor", sml::get_shift_factor(profile))));
        }

    } // namespace cfg
} // namespace smf
