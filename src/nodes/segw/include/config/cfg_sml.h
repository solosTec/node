/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_SML_H
#define SMF_SEGW_CONFIG_SML_H

#include <cfg.h>

namespace smf {

    class cfg_sml {
      public:
        cfg_sml(cfg &);
        cfg_sml(cfg_sml &) = default;

        boost::asio::ip::tcp::endpoint get_ep() const;
        boost::asio::ip::address get_address() const;
        std::uint16_t get_port() const;
        std::uint16_t get_discovery_port() const;
        std::string get_account() const;
        std::string get_pwd() const;
        bool is_enabled() const;

        /**
         * if true, server id must not match
         */
        bool accept_all_ids() const;

      private:
        cfg &cfg_;
    };

} // namespace smf

#endif
