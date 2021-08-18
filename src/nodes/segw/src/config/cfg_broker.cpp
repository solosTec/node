/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_broker.h>
#include <config/cfg_lmn.h>

#include <cyng/obj/container_factory.hpp>

namespace smf {

    cfg_broker::cfg_broker(cfg &c, lmn_type type)
        : cfg_(c)
        , type_(type) {}

    std::string cfg_broker::get_path_id() const { return std::to_string(get_index()); }

    bool cfg_broker::is_enabled() const { return cfg_lmn(cfg_, type_).is_broker_enabled(); }

    bool cfg_broker::has_login() const { return cfg_lmn(cfg_, type_).has_broker_login(); }

    bool cfg_broker::is_lmn_enabled() const { return cfg_lmn(cfg_, type_).is_enabled(); }

    std::string cfg_broker::get_port() const { return cfg_lmn(cfg_, type_).get_port(); }

    std::string cfg_broker::get_task_name() const { return "broker@" + get_port(); }

    namespace {
        std::string count_path(std::uint8_t type) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), "count");
        }
        std::string address_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "address");
        }
        std::string port_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "port");
        }
        std::string account_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "account");
        }
        std::string pwd_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "pwd");
        }
        std::string connect_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "connect-on-demand");
        }
        std::string timeout_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "write-timeout");
        }
        std::string watchdog_path(std::uint8_t type, std::size_t idx) {
            return cyng::to_path(cfg::sep, cfg_broker::root, std::to_string(type), idx, "watchdog");
        }
    } // namespace

    std::size_t cfg_broker::get_count() const { return cfg_.get_value(count_path(get_index()), static_cast<std::size_t>(0)); }

    std::string cfg_broker::get_address(std::size_t idx) const { return cfg_.get_value(address_path(get_index(), idx), ""); }

    std::uint16_t cfg_broker::get_port(std::size_t idx) const {
        return cfg_.get_value(port_path(get_index(), idx), static_cast<std::uint16_t>(12000));
    }

    std::string cfg_broker::get_account(std::size_t idx) const { return cfg_.get_value(account_path(get_index(), idx), ""); }

    std::string cfg_broker::get_pwd(std::size_t idx) const { return cfg_.get_value(pwd_path(get_index(), idx), ""); }

    bool cfg_broker::is_connect_on_demand(std::size_t idx) const { return cfg_.get_value(connect_path(get_index(), idx), true); }

    std::chrono::seconds cfg_broker::get_timeout(std::size_t idx) const {
        auto const to = cfg_.get_value(timeout_path(get_index(), idx), 1);
        //  at least 1 second
        return std::chrono::seconds((to < 1) ? 1 : to);
    }

    std::chrono::seconds cfg_broker::get_watchdog(std::size_t idx) const {
        auto const wd = cfg_.get_value(watchdog_path(get_index(), idx), 12);
        //  at least 5 seconds
        return std::chrono::seconds((wd < 5) ? 5 : wd);
    }

    std::string cfg_broker::get_login_sequence(std::size_t idx) const { return get_account(idx) + ":" + get_pwd(idx); }

    target cfg_broker::get_target(std::size_t idx) const {
        return target(
            get_account(idx),
            get_pwd(idx),
            get_address(idx),
            get_port(idx),
            is_connect_on_demand(idx),
            get_timeout(idx),
            get_watchdog(idx),
            idx);
    }

    target_vec cfg_broker::get_all_targets() const {
        target_vec r;

        for (std::size_t idx = 0; idx < get_count(); ++idx) {
            r.push_back(get_target(idx));
        }
        return r;
    }

    cyng::vector_t cfg_broker::get_target_vector() const {

        cyng::vector_t vec;
        auto const brokers = get_all_targets();
        std::transform(std::begin(brokers), std::end(brokers), std::back_inserter(vec), [](target const &srv) {
            return cyng::make_object(to_param_map(srv));
        });
        return vec;
    }

    bool cfg_broker::set_address(std::size_t idx, std::string address) const {
        return cfg_.set_value(address_path(get_index(), idx), address);
    }

    bool cfg_broker::set_port(std::size_t idx, std::uint16_t port) const {
        return cfg_.set_value(port_path(get_index(), idx), port);
    }

    bool cfg_broker::set_account(std::size_t idx, std::string account) const {
        return cfg_.set_value(account_path(get_index(), idx), account);
    }

    bool cfg_broker::set_pwd(std::size_t idx, std::string pwd) const { return cfg_.set_value(pwd_path(get_index(), idx), pwd); }

    bool cfg_broker::set_count(std::size_t size) const { return cfg_.set_value(count_path(get_index()), size); }

    std::size_t cfg_broker::update_count() {

        std::size_t count{0};
        for (;; ++count) {
            if (get_address(count).empty())
                break;
        }
        set_count(count);
        return count;
    }

    target::target(
        std::string account,
        std::string pwd,
        std::string address,
        std::uint16_t port,
        bool on_demand,
        std::chrono::seconds timeout,
        std::chrono::seconds watchdog,
        std::size_t index)
        : account_(account)
        , pwd_(pwd)
        , address_(address)
        , port_(port)
        , on_demand_(on_demand)
        , timeout_(timeout)
        , watchdog_(watchdog)
        , index_(index) {}

    std::string const &target::get_account() const { return account_; }

    std::string const &target::get_pwd() const { return pwd_; }
    bool target::is_connect_on_demand() const { return on_demand_; }
    std::chrono::seconds target::get_timeout() const { return timeout_; }
    std::chrono::seconds target::get_watchdog() const { return watchdog_; }
    std::size_t target::get_index() const { return index_; }

    std::string const &target::get_address() const { return address_; }

    std::uint16_t target::get_port() const { return port_; }

    std::string target::get_login_sequence() const { return account_ + ":" + pwd_; }

    std::ostream &operator<<(std::ostream &os, target const &brk) {
        if (!brk.get_account().empty()) {
            os << '#' << brk.get_index() << ' ' << brk.get_account() << ':' << brk.get_pwd() << '@';
        }
        os << brk.get_address() << ':' << brk.get_port() << (brk.is_connect_on_demand() ? "*" : "+");
        return os;
    }

    cyng::param_map_t to_param_map(target const &srv) {
        return cyng::param_map_factory("address", srv.get_address())("port", srv.get_port())("account", srv.get_account())(
            "pwd", srv.get_pwd())("connect-on-demand", srv.is_connect_on_demand());
    }

} // namespace smf
