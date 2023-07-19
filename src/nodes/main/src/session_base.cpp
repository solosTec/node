#include <session_base.hpp>

#include <cyng/parse/string.h>

#include <boost/algorithm/string.hpp>

namespace smf {
    std::chrono::seconds smooth(std::chrono::seconds interval) {
        auto const mod = interval.count() % 300;
        return std::chrono::seconds(interval.count() + (300 - mod));
    }

    std::pair<std::string, bool> is_custom_gateway(std::string const &id) {
        if (boost::algorithm::starts_with(id, "smf.gw:") || boost::algorithm::starts_with(id, "EMH-")) {
            auto const vec = cyng::split(id, ":");
            if (vec.size() == 3 && vec.at(2).size() == 14) {
                return {vec.at(2), true};
            }
            return {"05000000000001", true};
        } else if (id.size() == 15 && boost::algorithm::starts_with(id, "a") && boost::algorithm::ends_with(id, "en")) {
            // example: a00153B01E5B7en
            return {id.substr(1, 12), true};
        }
        return {"", false};
    }
} // namespace smf
