#include <smf/report/config/cfg_report.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>

#include <filesystem>

#include <boost/assert.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace cfg {
        report::report(cyng::param_map_t &&pm) noexcept
            : pm_(std::move(pm)) {}

        std::string report::get_db() const {
            auto const pos = pm_.find("db");
            return (pos != pm_.end()) ? cyng::value_cast(pos->second, "default") : "";
        }

        cyng::param_map_t report::get_profile_section() const {
            auto const pos = pm_.find("profiles");
            return (pos != pm_.end()) ? cyng::container_cast<cyng::param_map_t>(pos->second) : cyng::param_map_t{};
        }

        cyng::param_map_t report::get_config(cyng::obis code) const {
            auto const profiles = get_profile_section();
            auto const pos = profiles.find(cyng::to_string(code));
            return (pos != profiles.end()) ? cyng::container_cast<cyng::param_map_t>(pos->second) : cyng::param_map_t{};
        }

        std::set<cyng::obis> report::get_profiles() const {
            std::set<cyng::obis> r;
            auto const profiles = get_profile_section();
            std::transform(profiles.begin(), profiles.end(), std::inserter(r, r.end()), [](cyng::param_map_t::value_type const &v) {
                return cyng::to_obis(v.first);
            });
            BOOST_ASSERT(profiles.size() >= r.size());
            return r;
        }

        bool report::is_enabled(cyng::obis code) const {
            auto const config = get_config(code);
            auto const pos = config.find("enabled");
            return (pos != config.end()) ? cyng::value_cast(pos->second, false) : false;
        }

        std::string report::get_name(cyng::obis code) const {
            auto const config = get_config(code);
            auto const pos = config.find("name");
            return (pos != config.end()) ? cyng::value_cast(pos->second, "") : "";
        }

        std::string report::get_path(cyng::obis code) const {
            auto const cwd = std::filesystem::current_path().string();
            auto const config = get_config(code);
            auto const pos = config.find("path");
            return (pos != config.end()) ? cyng::value_cast(pos->second, cwd) : cwd;
        }

        std::string report::get_prefix(cyng::obis code) const {
            auto const config = get_config(code);
            auto const pos = config.find("prefix");
            return (pos != config.end()) ? cyng::value_cast(pos->second, "") : "";
        }

        std::chrono::hours report::get_backtrack(cyng::obis code) const {
            auto const config = get_config(code);
            auto const pos = config.find("backtrack");
            return (pos != config.end()) ? cyng::to_hours(cyng::value_cast(pos->second, "120:00:00")) : std::chrono::hours(0);
        }

    } // namespace cfg
} // namespace smf
