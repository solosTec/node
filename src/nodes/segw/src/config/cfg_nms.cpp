/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_nms.h>

#include <cyng/sys/net.h>

#include <iostream>

#include <boost/predef.h>

namespace smf {

    namespace { //  static linkage
        std::string address_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "address"); }
        std::string port_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "port"); }
        std::string account_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "account"); }
        std::string pwd_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "pwd"); }
        std::string enabled_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "enabled"); }
        std::string debug_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "debug"); }
        std::string script_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "script-path"); }
        std::string nic_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "nic"); }
        std::string nic_v4_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "nic-ipv4"); }
        std::string nic_linklocal_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "nic-linklocal"); }
        std::string nic_index_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "nic-index"); }
        std::string delay_path() { return cyng::to_path(cfg::sep, cfg_nms::root, "delay"); }
    } // namespace

    cfg_nms::cfg_nms(cfg &c)
        : cfg_(c) {}

    //	nms/account|operator|operator|14|NMS login name
    //	nms/address|0.0.0.0|0.0.0.0|14|NMS bind address
    //	nms/enabled|false|false|1|NMS enabled
    //	nms/port|1c5d|1c5d|7|NMS listener port (7261)
    //	nms/pwd|operator|operator|14|NMS login password
    //	nms/script-path|"C:\\Users\\WWW\\AppData\\Local\\Temp\\update-script.cmd"|"C:\\Users\\Sylko\\AppData\\Local\\Temp\\update-script.cmd"|15|path
    // to update script

    boost::asio::ip::tcp::endpoint cfg_nms::get_ep() const { return {boost::asio::ip::make_address(get_address()), get_port()}; }

    std::string cfg_nms::get_address() const { return cfg_.get_value(address_path(), "0.0.0.0"); }

    bool cfg_nms::set_address(std::string address) const { return cfg_.set_value(address_path(), address); }

    std::uint16_t cfg_nms::get_port() const { return cfg_.get_value(port_path(), get_default_nms_port()); }

    bool cfg_nms::set_port(std::uint16_t port) const { return cfg_.set_value(port_path(), port); }

    std::string cfg_nms::get_account() const { return cfg_.get_value(account_path(), ""); }

    bool cfg_nms::set_account(std::string account) const { return cfg_.set_value(account_path(), account); }

    std::string cfg_nms::get_pwd() const { return cfg_.get_value(pwd_path(), ""); }

    bool cfg_nms::set_pwd(std::string pwd) const { return cfg_.set_value(pwd_path(), pwd); }

    bool cfg_nms::is_enabled() const { return cfg_.get_value(enabled_path(), false); }

    bool cfg_nms::set_enabled(bool b) const { return cfg_.set_value(enabled_path(), b); }

    bool cfg_nms::is_debug() const { return cfg_.get_value(debug_path(), false); }

    std::chrono::seconds cfg_nms::get_delay() const { return cfg_.get_value(delay_path(), std::chrono::seconds(12)); }

    std::string cfg_nms::get_nic() const { return cfg_.get_value(nic_path(), smf::get_nic()); }

    boost::asio::ip::address cfg_nms::get_nic_ipv4() const { return cfg_.get_value(nic_v4_path(), get_ipv4_address(get_nic())); }
    boost::asio::ip::address cfg_nms::get_nic_linklocal() const {
        auto const r = get_ipv6_linklocal(get_nic());
        return cfg_.get_value(nic_linklocal_path(), r.first);
    }

    std::uint32_t cfg_nms::get_nic_index() const {
        auto const r = get_ipv6_linklocal(get_nic());
        return cfg_.get_value(nic_index_path(), r.second);
    }

    boost::asio::ip::address cfg_nms::get_linklocal_scoped() const {
        auto const r = get_nic_linklocal();
        return boost::asio::ip::make_address_v6(r.to_v6().to_bytes(), get_nic_index());
    }

    bool cfg_nms::set_delay(std::chrono::seconds delay) const { return cfg_.set_value(delay_path(), delay); }

    bool cfg_nms::set_debug(bool b) const { return cfg_.set_value(debug_path(), b); }

    std::filesystem::path cfg_nms::get_script_path() const {
        return cfg_.get_value(
            script_path(),
#if defined(BOOST_OS_LINUX_AVAILABLE)
            std::filesystem::path("/tmp/update-script.sh")
#else
            std::filesystem::temp_directory_path()
#endif
        );
    }

    bool cfg_nms::check_credentials(std::string const &user, std::string const &pwd) {
        return boost::algorithm::equals(user, get_account()) && boost::algorithm::equals(pwd, get_pwd());
    }

    std::string get_nic() {

#if defined(BOOST_OS_LINUX_AVAILABLE)
#if defined(__CROSS_PLATFORM)
        std::string const preferred = "br0";
#else
        std::string const preferred = "eth0";
#endif
#else
        std::string const preferred = "Ethernet";
#endif
        auto const nics = cyng::sys::get_nic_names();
        if (std::find(std::begin(nics), std::end(nics), preferred) == nics.end()) {
            std::cerr << "device: " << preferred << " not available" << std::endl;
            //
            //  select an available device.
            //  (1) try the the device of the first available IPv4 address
            //
            auto const cfg_v4 = cyng::sys::get_ipv4_configuration();
            if (!cfg_v4.empty()) {
                std::cout << "use " << cfg_v4.front().device_ << " instead" << std::endl;
                return cfg_v4.front().device_;
            }

            //
            //  (2) take the first available device
            //
            if (!nics.empty()) {
                std::cout << "use " << nics.front() << " instead" << std::endl;
                return nics.front();
            }
        }
        return preferred;
    }

    std::uint16_t get_default_nms_port() { return 7562; }

    boost::asio::ip::address get_ipv4_address(std::string const &nic) {
        auto const cfg_v4 = cyng::sys::get_ipv4_configuration();
        auto const cfg_filtered = cyng::sys::filter(cfg_v4, cyng::sys::filter_by_name(nic));
        return (cfg_filtered.empty()) ? boost::asio::ip::address_v4() : cfg_filtered.front().address_;
    }

    std::pair<boost::asio::ip::address, std::uint32_t> get_ipv6_linklocal(std::string const &nic) {
        auto const cfg_v6 = cyng::sys::get_ipv6_configuration();
        auto const cfg_filtered = cyng::sys::filter(cfg_v6, cyng::sys::filter_by_name(nic));
        for (auto const &cfg : cfg_filtered) {
            if (cfg.address_.to_v6().is_link_local())
                return {cfg.address_.to_v6(), cfg.index_};
        }
        return {boost::asio::ip::address_v6(), 0};
    }

} // namespace smf
