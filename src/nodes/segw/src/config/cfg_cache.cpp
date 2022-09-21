#include <config/cfg_cache.h>

#include <config/cfg_broker.h>
#include <config/cfg_lmn.h>

#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/string.h>

namespace smf {

    cfg_cache::cfg_cache(cfg &c, lmn_type type)
        : cfg_(c)
        , type_(type) {}

    namespace {
        std::string enabled_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_cache::root, std::to_string(type), "enabled");
        }
        std::string push_server_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_cache::root, std::to_string(type), "push-server");
        }
        std::string delay_path(std::size_t type) { return cyng::to_path(cfg::sep, cfg_cache::root, std::to_string(type), "delay"); }
    } // namespace

    bool cfg_cache::is_enabled() const {
        return cfg_.get_value(
            enabled_path(get_index()),
#ifdef _DEBUG
            true
#else
            false
#endif
        );
    }

    std::string cfg_cache::get_push_server() const { return cfg_.get_value(push_server_path(get_index()), "example.com:2222"); }

    std::string cfg_cache::get_push_host() const {
        auto const server = get_push_server();
        auto const vec = cyng::split(server, ":");
        return (vec.size() == 2) ? vec.at(0) :
#ifdef _DEBUG
                                 "localhost"
#else
                                 ""
#endif
            ;
    }

    std::string cfg_cache::get_push_service() const {
        auto const server = get_push_server();
        auto const vec = cyng::split(server, ":");
        return (vec.size() == 2) ? vec.at(1) :
#ifdef _DEBUG
                                 "2222";
#else
                                 "0";
#endif
        ;
    }

    std::chrono::minutes cfg_cache::get_period() const {
        return cfg_.get_value(delay_path(get_index()), std::chrono::minutes(60u));
    }

    bool cfg_cache::set_enabled(bool b) { return cfg_.set_value(enabled_path(get_index()), b); }

    bool cfg_cache::set_push_server(std::string server) { return cfg_.set_value(push_server_path(get_index()), server); }

    bool cfg_cache::set_period(std::chrono::minutes d) { return cfg_.set_value(delay_path(get_index()), d); }

} // namespace smf
