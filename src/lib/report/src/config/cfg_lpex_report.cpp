#include <smf/report/config/cfg_lpex_report.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>

#include <boost/assert.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        lpex_report::lpex_report(cyng::param_map_t &&pm) noexcept
            : report(std::move(pm)) {}

        bool lpex_report::is_debug_mode() const {
            auto const pos = pm_.find("debug");
            return (pos != pm_.end()) ? cyng::value_cast(pos->second, false) : false;
        }

        bool lpex_report::is_print_version() const {
            auto const pos = pm_.find("print.version");
            return (pos != pm_.end()) ? cyng::value_cast(pos->second, true) : true;
        }

        bool lpex_report::add_customer_data(cyng::obis code) const {
            auto const config = get_config(code);
            auto const pos = config.find("add.customer.data");
            return (pos != config.end()) ? cyng::value_cast(pos->second, false) : false;
        }

        cyng::obis_path_t lpex_report::get_filter() const {
            auto const pos = pm_.find("filter");
            return (pos != pm_.end()) ? cyng::to_obis_path(cyng::value_cast(pos->second, "")) : cyng::obis_path_t{};
        }

    } // namespace cfg
} // namespace smf
