#include <smf/report/config/cfg_csv_report.h>

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
        csv_report::csv_report(cyng::param_map_t &&pm) noexcept
            : report(std::move(pm)) {}

        // std::set<cyng::obis> csv_report::get_profiles() const {
        //     std::set<cyng::obis> r;
        //     std::transform(pm_.begin(), pm_.end(), std::inserter(r, r.end()), [](cyng::param_map_t::value_type const &v) {
        //         return cyng::to_obis(v.first);
        //     });
        //     BOOST_ASSERT(pm_.size() >= r.size());
        //     return r;
        // }

    } // namespace cfg
} // namespace smf
