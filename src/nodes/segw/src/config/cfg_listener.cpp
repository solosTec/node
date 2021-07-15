/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_listener.h>
#include <config/cfg_lmn.h>

#include <cyng/sys/net.h>

#include <cyng/obj/container_factory.hpp>

namespace smf {

    cfg_listener::cfg_listener(cfg &c, lmn_type type)
        : cfg_(c)
        , type_(type) {}

    std::string cfg_listener::get_path_id() const { return std::to_string(get_index()); }

    bool cfg_listener::is_lmn_enabled() const { return cfg_lmn(cfg_, type_).is_enabled(); }

    std::string cfg_listener::get_port_name() const { return cfg_lmn(cfg_, type_).get_port(); }

    std::string cfg_listener::get_task_name() const { return std::string(get_name(type_)) + ":" + get_port_name(); }

    namespace {
        std::string address_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "address");
        }
        std::string port_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "port");
        }
        std::string enabled_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "enabled");
        }
        std::string login_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "login");
        }
        std::string delay_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "delay");
        }
        std::string timeout_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "timeout");
        }
        std::string task_id_ipv4_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "taskIdIPv4");
        }
        std::string task_id_ipv6_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "taskIdIPv6");
        }
#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)
        std::string nic_path(std::uint8_t type) { return cyng::to_path(cfg::sep, cfg_listener::root, std::to_string(type), "nic"); }
#endif
    } // namespace

    boost::asio::ip::address cfg_listener::get_address() const {
        return cfg_.get_value(address_path(get_index()), boost::asio::ip::address());
    }

    std::uint16_t cfg_listener::get_port() const {
        return cfg_.get_value(port_path(get_index()), static_cast<std::uint16_t>(12000));
    }

    std::chrono::seconds cfg_listener::get_delay() const {
        return cfg_.get_value(delay_path(get_index()), std::chrono::seconds(20));
    }

    boost::posix_time::seconds cfg_listener::get_timeout() const {
        return boost::posix_time::seconds(cfg_.get_value(timeout_path(get_index()), 10));
    }

    boost::asio::ip::tcp::endpoint cfg_listener::get_ipv4_ep() const { return {get_address(), get_port()}; }

    std::size_t cfg_listener::get_IPv4_task_id() const {
        return cfg_.get_value(task_id_ipv4_path(get_index()), static_cast<std::size_t>(0));
    }
    std::size_t cfg_listener::get_IPv6_task_id() const {
        return cfg_.get_value(task_id_ipv6_path(get_index()), static_cast<std::size_t>(0));
    }

    bool cfg_listener::is_enabled() const { return cfg_.get_value(enabled_path(get_index()), false); }

    bool cfg_listener::has_login() const { return cfg_.get_value(login_path(get_index()), false); }

#if defined(__CROSS_PLATFORM) && defined(BOOST_OS_LINUX_AVAILABLE)

    std::string cfg_listener::get_nic() const { return cfg_.get_value(nic_path(get_index()), "ens33"); }

    boost::asio::ip::tcp::endpoint cfg_listener::get_ipv6_ep(std::string nic) {
        auto const pres = cyng::sys::get_nic_prefix();
        auto const pos = std::find(pres.begin(), pres.end(), nic);

        if (pos != pres.end()) {

            //  this is something like: fe80::225:18ff:fea4:9982%32
            std::string local_address_with_scope = cyng::sys::get_address_IPv6(nic, cyng::sys::LINKLOCAL).to_string();

            //
            //  substitute %nn with % nic
            //
            auto const pos = local_address_with_scope.find_last_of('%');
            if (pos != std::string::npos) {
                local_address_with_scope = local_address_with_scope.substr(0, pos + 1) + nic;

                return boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(local_address_with_scope), get_port());
            }
        }

        return boost::asio::ip::tcp::endpoint();
    }
#endif

    bool cfg_listener::set_address(std::string address) const { return cfg_.set_value(address_path(get_index()), address); }

    bool cfg_listener::set_port(std::uint16_t port) const { return cfg_.set_value(port_path(get_index()), port); }

    bool cfg_listener::set_login(bool b) const { return cfg_.set_value(login_path(get_index()), b); }

    bool cfg_listener::set_enabled(bool b) const { return cfg_.set_value(enabled_path(get_index()), b); }

    bool cfg_listener::set_delay(std::chrono::seconds timeout) const { return cfg_.set_value(delay_path(get_index()), timeout); }
    bool cfg_listener::set_timeout(int timeout) const { return cfg_.set_value(timeout_path(get_index()), timeout); }

    bool cfg_listener::set_IPv4_task_id(std::size_t id) const { return cfg_.set_value(task_id_ipv4_path(get_index()), id); }
    bool cfg_listener::set_IPv6_task_id(std::size_t id) const { return cfg_.set_value(task_id_ipv6_path(get_index()), id); }

    bool cfg_listener::remove_IPv4_task_id() { return cfg_.remove_value(task_id_ipv4_path(get_index())); }
    bool cfg_listener::remove_IPv6_task_id() { return cfg_.remove_value(task_id_ipv6_path(get_index())); }

    std::ostream &operator<<(std::ostream &os, cfg_listener const &cfg) {
        os << cfg.get_port_name() << '@' << cfg.get_address() << ':' << cfg.get_port();
        return os;
    }

} // namespace smf
