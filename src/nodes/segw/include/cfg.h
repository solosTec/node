/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CFG_H
#define SMF_SEGW_CFG_H

#include <smf/sml/status.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/value_cast.hpp>
#include <cyng/store/db.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    /**
     * manage configuration data
     */
    class bridge;
    class cfg {
        friend class bridge;

      public:
        cfg(cyng::logger, cyng::store &);

        inline cyng::store &get_cache() { return cache_; }

        /**
         * @return itentity/source tag
         */
        boost::uuids::uuid get_tag() const;

        /**
         * get configured server ID (OBIS_SERVER_ID:)
         * 05 + MAC
         */
        cyng::buffer_t get_srv_id() const;

        /**
         * read a configuration object from table "cfg"
         */
        cyng::object get_obj(std::string name);

        /**
         * read a configuration value from table "_Cfg"
         */
        template <typename T> T get_value(std::string name, T def) {
            if constexpr (std::is_arithmetic_v<T>) {
                return cyng::numeric_cast<T>(get_obj(name), std::forward<T>(def));
            }
            return cyng::value_cast(get_obj(name), def);
        }

        /**
         * The non-template function wins.
         */
        std::string get_value(std::string name, const char *def) { return cyng::value_cast(get_obj(name), std::string(def)); }

        /**
         * set/insert a configuration value
         */
        bool set_obj(std::string name, cyng::object &&obj);
        bool set_value(std::string name, cyng::object obj);

        /**
         * remove the value with the specified key/path
         */
        bool remove_value(std::string name);

        template <typename T> bool set_value(std::string name, T value) {
            return set_obj(name, cyng::make_object(std::move(value)));
        }

        /**
         * @return SML status word
         */
        sml::status_word_t get_status_word() const;

        /**
         * loop over all elements
         */
        void loop(std::function<void(std::vector<std::string> &&, cyng::object)> cb);

        /**
         * loop over all elements filtered
         */
        void loop(std::string const &filter, std::function<void(std::vector<std::string> &&, cyng::object)> cb);

        /**
         * the separator character
         */
        constexpr static char sep = '/';

      private:
        cyng::logger logger_;
        cyng::store &cache_;

        /**
         * source tag - initialized by bridge
         */
        boost::uuids::uuid tag_;

        /**
         * server id
         * 05 + MAC
         */
        cyng::buffer_t id_;

        /**
         * OBIS log status
         */
        sml::status_word_t status_word_;
    };
} // namespace smf

#endif
