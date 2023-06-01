/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_CONFIG_H
#define SMF_IPT_BUS_CONFIG_H

#include <smf/ipt/scramble_key.h>

#include <cyng/obj/intrinsics/container.h>

#include <chrono>
#include <ostream>
#include <string>

namespace smf {
    namespace ipt {
        /**
         * convience class to hold all data required to manage an ip-t connection
         */
        struct server {
            server();
            server(server const &) = default;
            server(
                std::string const &,
                std::string const &,
                std::string const &,
                std::string const &,
                scramble_key const &,
                bool,
                int);

            std::string const host_;
            std::string const service_;
            std::string const account_;
            std::string const pwd_;
            scramble_key const sk_; //!< default scramble key
            bool const scrambled_;
            std::chrono::seconds const monitor_;
        };

        /**
         * debug helper
         */
        std::ostream &operator<<(std::ostream &os, server const &);

        /**
         * read a configuration map with "service", "host", "account", "pwd",
         * and "salt"
         */
        server read_config(cyng::param_map_t const &pmap);

        class toggle {
          public:
            /**
             * Declare a vector with all cluster configurations.
             */
            using server_vec_t = std::vector<server>;

          public:
            toggle(server_vec_t &&);
            toggle(toggle const &) = default;

            /**
             * Switch redundancy
             */
            server changeover() const;

            /**
             * get current reduncancy
             */
            server get() const;

          private:
            server_vec_t const cfg_;
            mutable std::size_t active_; //!<	index of active configuration
        };

        /**
         * read config vector
         */
        toggle::server_vec_t read_config(cyng::vector_t const &vec);

        /**
         * convience class to hold all data required to manage an ip-t channel
         */
        class push_channel {
          public:
            push_channel() = default;
            push_channel(push_channel const &) = default;
            push_channel(
                std::string const &,
                std::string const &,
                std::string const &,
                std::string const &,
                std::string const &,
                std::uint16_t);

            friend std::ostream &operator<<(std::ostream &, push_channel const &);

            /**
             * @return push channel configuration as tuple
             */
            auto as_tuple() const -> std::tuple<std::string, std::string, std::string, std::string, std::string, std::uint16_t>;

            /**
             * @return true if the target name is not empty.
             */
            bool is_valid() const noexcept;

            /*
             * @return ipt target name
             */
            std::string const &get_target() const noexcept;

          private:
            std::string target_;
            std::string account_;
            std::string number_;
            std::string id_;
            std::string version_;
            std::uint16_t timeout_;
        };

        push_channel read_push_channel_config(cyng::param_map_t const &pmap);

    } // namespace ipt

} // namespace smf

#endif
