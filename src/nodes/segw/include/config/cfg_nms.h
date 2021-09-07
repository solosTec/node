/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_NMS_H
#define SMF_SEGW_CONFIG_NMS_H

#include <cfg.h>
#include <filesystem>

namespace smf {

    class cfg_nms {
      public:
        enum class mode {
            PRODUCTION, //  link-local address
            TEST,       //  0.0.0.0
            LOCAL       //  IPv4 address
        };

      public:
        cfg_nms(cfg &);
        cfg_nms(cfg_nms &) = default;

        /**
         * @return default IPv4 listener address
         */
        std::string get_address() const;
        std::uint16_t get_port() const;

        /**
         * @return IP endpoint build from "nms/address" and "nms/port"
         */
        boost::asio::ip::tcp::endpoint get_ep() const;

        std::string get_account() const;
        std::string get_pwd() const;
        bool is_enabled() const;
        bool is_debug() const;
        /**
         * listener rebind()
         */
        std::chrono::seconds get_delay() const;

        std::string get_mode_name() const;

        /**
         * Designated NIC for link-local communication
         * @note These nic releated functions are slow since every call queries
         * the network configuration of the system.
         */
        std::string get_nic() const;

        /**
         * @return first IPv4 address of the specified nic
         */
        boost::asio::ip::address get_nic_ipv4() const;

        /**
         * @return link-local (IPv6) address of the specified nic
         */
        // boost::asio::ip::address get_nic_ipv6() const;

        /**
         * @return device index of the specified nic
         */
        std::uint32_t get_nic_index() const;

        /**
         * Same as get_nic_ipv6() but with an already scoped IPv6 address.
         * This is always an IPv6 address since IPv4 doesn't have a concept of scoped
         * addresses.
         */
        boost::asio::ip::address get_nic_linklocal() const;
        boost::asio::ip::tcp::endpoint get_nic_linklocal_ep() const;

        /**
         * check username and password
         */
        bool check_credentials(std::string const &, std::string const &);

        std::filesystem::path get_script_path() const;

        /**
         * The address has to a string since it can contains the additional
         * information of the interface name.
         * example: fe80::225:18ff:fea0:a3b%br0
         */
        bool set_address(std::string) const;
        bool set_port(std::uint16_t port) const;
        bool set_account(std::string) const;
        bool set_pwd(std::string) const;
        bool set_enabled(bool) const;
        bool set_debug(bool) const;
        bool set_delay(std::chrono::seconds) const;

        constexpr static char root[] = "nms";

      private:
        cfg &cfg_;
    };

    /**
     * @return the preferred network device to communicate with.
     */
    std::string get_nic();

    /**
     * @return the preferred IP port number for NMS,
     */
    std::uint16_t get_default_nms_port();

    /**
     * @return first IPv4 address of specified device
     */
    boost::asio::ip::address get_ipv4_address(std::string const &);

    /**
     * @return link-local (IPv6) address of specified device
     */
    std::pair<boost::asio::ip::address, std::uint32_t> get_ipv6_linklocal(std::string const &);

    /**
     * @return the enum of one of the following strings: production, test, local
     */
    cfg_nms::mode get_mode(std::string);

} // namespace smf

#endif
