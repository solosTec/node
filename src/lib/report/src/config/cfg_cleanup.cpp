#include <smf/report/config/cfg_cleanup.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>

#include <boost/assert.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        cleanup::cleanup(cyng::param_map_t &&pm) noexcept
            : pm_(std::move(pm)) {}

        std::set<cyng::obis> cleanup::get_profiles() const {
            std::set<cyng::obis> r;
            std::transform(pm_.begin(), pm_.end(), std::inserter(r, r.end()), [](cyng::param_map_t::value_type const &v) {
                return cyng::to_obis(v.first);
            });
            BOOST_ASSERT(pm_.size() == r.size());
            return r;
        }

        bool cleanup::is_enabled(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("enabled");
                return (p != config.end()) ? cyng::value_cast(pos->second, false) : false;
            }
            return false;
        }

        std::string cleanup::get_name(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("name");
                return (p != config.end()) ? cyng::value_cast(pos->second, "") : "";
            }
            return "";
        }
        std::chrono::hours cleanup::get_max_age(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("max.age");
                return (p != config.end()) ? cyng::to_hours(cyng::value_cast(p->second, "120:00:00")) : std::chrono::hours(0);
            }
            return std::chrono::hours(0);
        }

    } // namespace cfg
} // namespace smf
