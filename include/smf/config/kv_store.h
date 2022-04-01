/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_KV_STORE_H
#define SMF_CONFIG_KV_STORE_H

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/value_cast.hpp>
#include <cyng/store/db.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    /**
     * manage configuration data
     */
    class kv_store {
      public:
        kv_store(cyng::store &, std::string table_name, boost::uuids::uuid tag);

        /**
         * use table name "config"
         */
        kv_store(cyng::store &, boost::uuids::uuid tag);

        /**
         * read a configuration object from table "kv_store"
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
        std::string get_value(std::string name, const char *def);

        /**
         * set/insert a configuration value
         */
        bool set_obj(std::string name, cyng::object &&obj);
        bool set_value(std::string name, cyng::object obj);

        template <typename T> bool set_value(std::string name, T value) {
            return set_obj(name, cyng::make_object(std::move(value)));
        }

        /**
         * remove the value with the specified key/path
         */
        bool remove_value(std::string name);

        /**
         * @return identity/source tag
         */
        boost::uuids::uuid get_tag() const;

      private:
        cyng::store &cache_;

        /**
         * name of referenced table.
         */
        std::string const table_name_;

        /**
         * source tag
         */
        boost::uuids::uuid const tag_;
    };

    /**
     * Line up config data types
     */
    cyng::object tidy_config(std::string key, cyng::object &value);

} // namespace smf

#endif
