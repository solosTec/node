/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_HTTP_POST_H
#define SMF_SEGW_CONFIG_HTTP_POST_H

#include <config/cfg_lmn.h>

namespace smf {

    class cfg_http_post {
      public:
        cfg_http_post(cfg &, lmn_type);
        cfg_http_post(cfg_http_post &) = default;

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
        std::string get_server() const;

        /**
         * @return only the host name or address
         */
        std::string get_host() const;

        /**
         * @return the service/port
         */
        std::string get_service() const;

        /**
         * @return serial number (MAC)
         */
        std::string get_serial() const;

        /**
         * @return cycle time in minutes
         */
        std::chrono::minutes get_interval() const;

        bool set_enabled(bool b);

        /**
         * @param server must be formatted as "host:port"
         */
        bool set_push_server(std::string server);

        /**
         * @param serial must consist of 12 hex values
         */
        bool set_serial(std::string serial);

        /**
         * Set interval time
         * @return cycle time in minutes
         */
        bool set_interval(std::chrono::minutes);

        /**
         * Duplicate from cfg_lmn class
         *
         * @return name of serial port
         */
        std::string get_port() const;

        /**
         * format: "http.post@PORT
         */
        std::string get_task_name() const;

        /**
         * format: emt@PORT
         */
        std::string get_emt_task_name() const;

        constexpr static char root[] = "http.post";

      private:
        cfg &cfg_;
        lmn_type const type_;
    };
} // namespace smf

#endif
