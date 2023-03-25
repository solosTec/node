#include <smf/report/config/cfg_gap.h>

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
        gap::gap(cyng::param_map_t &&pm) noexcept
            : pm_(std::move(pm)) {}

        std::set<cyng::obis> gap::get_profiles() const {
            std::set<cyng::obis> r;
            std::transform(pm_.begin(), pm_.end(), std::inserter(r, r.end()), [](cyng::param_map_t::value_type const &v) {
                return cyng::to_obis(v.first);
            });
            BOOST_ASSERT(pm_.size() == r.size());
            return r;
        }

        bool gap::is_enabled(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("enabled");
                return (p != config.end()) ? cyng::value_cast(pos->second, false) : false;
            }
            return false;
        }
        std::string gap::get_name(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("name");
                return (p != config.end()) ? cyng::value_cast(pos->second, "") : "";
            }
            return "";
        }

        std::string gap::get_path(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("path");
                return (p != config.end()) ? cyng::value_cast(pos->second, "") : "";
            }
            return "";
        }

        std::chrono::hours gap::get_backtrack(cyng::obis code) const {
            auto const pos = pm_.find(cyng::to_string(code));
            if (pos != pm_.end()) {
                auto const config = cyng::container_cast<cyng::param_map_t>(pos->second);
                auto const p = config.find("path");
                return (p != config.end()) ? cyng::to_hours(cyng::value_cast(pos->second, "120:00:00")) : std::chrono::hours(0);
            }
            return std::chrono::hours(0);
        }

    } // namespace cfg
} // namespace smf
