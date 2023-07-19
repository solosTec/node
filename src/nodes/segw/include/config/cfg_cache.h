/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_CACHE_H
#define SMF_SEGW_CONFIG_CACHE_H

#include <config/cfg_lmn.h>

namespace smf {

    /**
     * The configuration data for the cache for reading data.
     */
    class cfg_cache {
      public:
        cfg_cache(cfg &, lmn_type);
        cfg_cache(cfg_cache &) = default;

        /**
         * numerical value of the specified LMN enum type
         */
        constexpr std::uint8_t get_index() const { return static_cast<std::uint8_t>(type_); }

        /**
         * Default for debug builds is true, otherwise false.
         */
        bool is_enabled() const;

        /**
         * @return server as "host:port"
         */
        std::string get_push_server() const;

        /**
         * @return only the host name or address
         */
        std::string get_push_host() const;

        /**
         * @return the service/port
         */
        std::string get_push_service() const;

        /**
         * @return cycle time in minutes
         */
        std::chrono::minutes get_interval() const;

        /**
         * Enable/disable cache for reading data.
         */
        bool set_enabled(bool b);

        /**
         * @param server must be formatted as "host:port"
         */
        bool set_push_server(std::string server);

        /**
         * Set cycle time
         */
        bool set_period(std::chrono::minutes);

        constexpr static char root[] = "cache";

      private:
        cfg &cfg_;
        lmn_type const type_;
    };
} // namespace smf

#endif
