#include <smf/report/config/gen_lpex.h>

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        cyng::tuple_t gen_lpex(std::filesystem::path cwd) {
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
                // 1-0:1.8.0*255    (01 00 01 08 00 FF)
                // 1-0:1.8.1*255    (01 00 01 08 01 FF)
                // 1-0:1.8.2*255    (01 00 01 08 02 FF)
                // 1-0:16.7.0*255   (01 00 10 07 00 FF)
                // 1-0:2.0.0*255    (01 00 02 00 00 FF)
                // 7-0:3.0.0*255    (07 00 03 00 00 FF) // Volume (meter), temperature converted (Vtc), forward, absolute,
                // current value 7-0:3.1.0*1      (07 00 03 01 00 01)
                // only this obis codes will be accepted
                cyng::make_param("filter", cyng::obis_path_t{OBIS_REG_POS_ACT_E, OBIS_REG_POS_ACT_E_T1, OBIS_REG_GAS_MC_0_0}),
                cyng::make_param(
                    "profiles",
                    cyng::make_tuple(
                        create_lpex_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                        create_lpex_spec(OBIS_PROFILE_60_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                        create_lpex_spec(OBIS_PROFILE_24_HOUR, cwd, false, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                        create_lpex_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH))) // lpex tuple
                    )                                                                                                  // lpex param
            );
        }
        cyng::prop_t create_lpex_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
            return cyng::make_prop(
                profile,
                cyng::make_tuple(
                    cyng::make_param("name", obis::get_name(profile)),
                    cyng::make_param("path", (cwd / "lpex.reports").string()),
                    cyng::make_param("backtrack", backtrack),
                    cyng::make_param("prefix", sml::get_prefix(profile)),
                    cyng::make_param("add.customer.data", false), // add/update customer data
                    cyng::make_param("enabled", enabled)));
        }
    } // namespace cfg
} // namespace smf
