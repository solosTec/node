/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_vmeter.h>
#include <smf/config/protocols.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {

    cfg_vmeter::cfg_vmeter(cfg &c, lmn_type type)
        : cfg_(c)
        , type_(type) {}

    namespace {
        std::string enabled_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_vmeter::root, std::to_string(type), "enabled");
        }
        std::string server_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_vmeter::root, std::to_string(type), "server");
        }
        std::string interval_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_vmeter::root, std::to_string(type), "interval");
        }
        std::string protocol_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_vmeter::root, std::to_string(type), "protocol");
        }
    } // namespace

    std::string cfg_vmeter::get_path_id() const { return std::to_string(get_index()); }

    bool cfg_vmeter::is_enabled() const { return cfg_.get_value(enabled_path(get_index()), false); }

    cyng::buffer_t cfg_vmeter::get_server() const {
        cyng::buffer_t tmp;
        return cfg_.get_value(server_path(get_index()), tmp);
    }

    std::string cfg_vmeter::get_protocol() const {
        return cfg_.get_value(protocol_path(get_index()), config::get_name(config::protocol::ANY));
    }
    std::chrono::seconds cfg_vmeter::get_interval() const {
        return cfg_.get_value(interval_path(get_index()), std::chrono::seconds(20));
    }

} // namespace smf
