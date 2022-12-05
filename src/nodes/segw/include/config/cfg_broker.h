/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_BROKER_H
#define SMF_SEGW_CONFIG_BROKER_H

#include <config/cfg_lmn.h>
#include <ostream>
#include <vector>

namespace smf {

    /**
     * helper class to establish a compact broker configuration
     */
    class target {
      public:
        target(
            std::string account,
            std::string pwd,
            std::string address,
            std::uint16_t port,
            bool on_demand,
            std::chrono::seconds timeout,
            std::chrono::seconds watchdog,
            std::size_t);

        std::string const &get_address() const;
        std::uint16_t get_port() const;
        std::string const &get_account() const;
        std::string const &get_pwd() const;
        bool is_connect_on_demand() const;
        std::chrono::seconds get_timeout() const;
        std::chrono::seconds get_watchdog() const;
        std::size_t get_index() const;

        std::string get_login_sequence() const;

      private:
        std::string const account_;
        std::string const pwd_;
        std::string const address_;
        std::uint16_t const port_;
        bool const on_demand_;
        std::chrono::seconds const timeout_;
        std::chrono::seconds const watchdog_;
        std::size_t const index_;
    };

    /**
     * debug helper
     * Adds a '*' at the end if connection is established when demandeda dn otherwise a '+'.
     */
    std::ostream &operator<<(std::ostream &os, target const &);

    /**
     * Convert to an parameter map
     */
    cyng::param_map_t to_param_map(target const &srv);

    /**
     * list of all broker (for one serial port)
     */
    using target_vec = std::vector<target>;

    class cfg_broker {
      public:
        cfg_broker(cfg &, lmn_type);
        cfg_broker(cfg_broker &) = default;

        /**
         * numerical value of the specified LMN enum type
         */
        constexpr std::uint8_t get_index() const { return static_cast<std::uint8_t>(type_); }

        std::string get_path_id() const;

        /**
         *  @brief LMN configuration
         *  lmn/N/broker-login
         */
        bool is_enabled() const;

        /**
         *  @brief LMN configuration
         *  lmn/N/broker-login
         */
        bool has_login() const;

        /**
         * A broker without an active LMN is virtually useless.
         * This method helps to detect such a misconfiguration.
         */
        bool is_lmn_enabled() const;

        /**
         * Duplicate from cfg_lmn class
         *
         * @return name of serial port
         */
        std::string get_port() const;

        /**
         * The task name allows to draw a connection between port and broker.
         * The name hast the following format:
         * "broker@<port>"
         *
         * All broker for this port have the same name.
         */
        std::string get_task_name() const;

        /**
         * There is no test if this value is correct.
         *
         * @return the specified number of broker configurations
         * for this serial port.
         */
        std::size_t get_count() const;

        std::string get_address(std::size_t idx) const;
        std::uint16_t get_port(std::size_t idx) const;
        std::string get_account(std::size_t idx) const;
        std::string get_pwd(std::size_t idx) const;

        /**
         * if not specified, the default value is true
         */
        bool is_connect_on_demand(std::size_t idx) const;

        /**
         * timeout after write (on-demand only)
         */
        std::chrono::seconds get_timeout(std::size_t idx) const;

        /**
         * watchdog timer for connection state (on-start only)
         */
        std::chrono::seconds get_watchdog(std::size_t idx) const;

        /**
         * account + ':' + pwd
         */
        std::string get_login_sequence(std::size_t idx) const;

        target get_target(std::size_t idx) const;
        target_vec get_all_targets() const;

        /**
         * same as get_all_targets()
         */
        cyng::vector_t get_target_vector() const;

        bool set_address(std::size_t idx, std::string) const;
        bool set_port(std::size_t idx, std::uint16_t) const;
        bool set_account(std::size_t idx, std::string) const;
        bool set_pwd(std::size_t idx, std::string) const;

        bool set_count(std::size_t size) const;

        /**
         * update the "broker/N/count" entry
         */
        std::size_t update_count();

        constexpr static char root[] = "broker";

      private:
        cfg &cfg_;
        lmn_type const type_;
    };
} // namespace smf

#endif
