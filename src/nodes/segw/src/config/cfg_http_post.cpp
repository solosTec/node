#include <config/cfg_http_post.h>

#include <config/cfg_broker.h>
#include <config/cfg_lmn.h>

#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/string.h>

namespace smf {

    cfg_http_post::cfg_http_post(cfg &c, lmn_type type)
        : cfg_(c)
        , type_(type) {}

    namespace {
        std::string enabled_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_http_post::root, std::to_string(type), "enabled");
        }
        std::string server_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_http_post::root, std::to_string(type), "server");
        }
        std::string serial_path(std::size_t type) {
            return cyng::to_path(cfg::sep, cfg_http_post::root, std::to_string(type), "serial");
        }
        std::string interval_path(std::size_t type) {
            return cyng::to_path(cfg::sep, cfg_http_post::root, std::to_string(type), "interval");
        }
    } // namespace

    bool cfg_http_post::is_enabled() const {
        return cfg_.get_value(
            enabled_path(get_index()),
#ifdef _DEBUG
            true
#else
            false
#endif
        );
    }

    std::string cfg_http_post::get_server() const { return cfg_.get_value(server_path(get_index()), "127.0.0.1:4455"); }

    std::string cfg_http_post::get_host() const {
        auto const server = get_server();
        auto const vec = cyng::split(server, ":");
        return (vec.size() == 2) ? vec.at(0) :
#ifdef _DEBUG
                                 "localhost"
#else
                                 ""
#endif
            ;
    }

    std::string cfg_http_post::get_service() const {
        auto const server = get_server();
        auto const vec = cyng::split(server, ":");
        return (vec.size() == 2) ? vec.at(1) :
#ifdef _DEBUG
                                 "4455";
#else
                                 "0";
#endif
        ;
    }

    std::string cfg_http_post::get_serial() const { return cfg_.get_value(serial_path(get_index()), "000000000000"); }

    std::chrono::minutes cfg_http_post::get_interval() const {
        return cfg_.get_value(interval_path(get_index()), std::chrono::minutes(20));
    }

    bool cfg_http_post::set_enabled(bool b) { return cfg_.set_value(enabled_path(get_index()), b); }

    bool cfg_http_post::set_push_server(std::string server) {
        auto const vec = cyng::split(server, ":");
        return (vec.size() == 2) ? cfg_.set_value(server_path(get_index()), server) : false;
    }

    bool cfg_http_post::set_serial(std::string s) { return (s.size() == 12) ? cfg_.set_value(serial_path(get_index()), s) : false; }

    bool cfg_http_post::set_interval(std::chrono::minutes d) { return cfg_.set_value(interval_path(get_index()), d); }

    std::string cfg_http_post::get_port() const { return cfg_lmn(cfg_, type_).get_port(); }

    std::string cfg_http_post::get_task_name() const { return std::string(root) + "@" + get_port(); }
    std::string cfg_http_post::get_emt_task_name() const { return "emt@" + get_port(); }

} // namespace smf
