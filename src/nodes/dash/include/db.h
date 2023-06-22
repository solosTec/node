/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_HTTP_DB_H
#define SMF_DASH_HTTP_DB_H

#include <smf/cluster/bus.h>
#include <smf/config/kv_store.h>
#include <smf/config/schemes.h>

#include <cyng/log/logger.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/store/db.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

    class notifier;
    class db : public cfg_db_interface {
        friend class notifier;

      public:
        /**
         * releation between table name, channel name and table size channel
         */
        struct rel {
            rel(std::string, std::string, std::string);
            rel() = default;
            rel(rel const &) = default;

            bool empty() const;

            std::string const table_;
            std::string const channel_;
            std::string const counter_;
        };

      public:
        db(cyng::store &cache, cyng::logger, boost::uuids::uuid tag);

        void init(
            std::uint64_t max_upload_size,
            std::string const &nickname,
            std::chrono::seconds timeout,
            std::string const &country_code,
            std::string const &lang_code,
            cyng::slot_ptr);

        /**
         * part of cfg_db_interface
         */
        virtual void
        db_res_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag)
            override;

        /**
         * part of cfg_db_interface
         */
        virtual void
        db_res_update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag)
            override;

        /**
         * part of cfg_db_interface
         */
        virtual void db_res_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) override;

        /**
         * part of cfg_db_interface
         */
        virtual void db_res_clear(std::string table_name, boost::uuids::uuid tag) override;

        /**
         * part of cfg_db_interface
         */
        virtual void db_res_trx(std::string, bool) override;

        /**
         * search a relation record by table name
         */
        rel by_table(std::string const &name) const;

        /**
         * search a relation record by channel name
         */
        rel by_channel(std::string const &name) const;

        /**
         * search a relation record by counter name
         */
        rel by_counter(std::string const &name) const;

        /**
         * loop over all rel-entries
         */
        void loop_rel(std::function<void(rel const &)>) const;

        void add_to_subscriptions(std::string const &channel, boost::uuids::uuid tag);

        /**
         * remove (a ws) from the subscription list
         *
         * @return number of affected subscriptions
         */
        std::size_t remove_from_subscription(boost::uuids::uuid tag);

        /**
         * convert data types from ws to matching data types
         * in table schemes.
         */
        void convert(std::string const &table_name, cyng::vector_t &key, cyng::param_map_t &data);

        /**
         * convert data types from ws to matching data types
         * in table schemes.
         */
        void convert(std::string const &table_name, cyng::vector_t &key);

        /**
         * To insert a new entity over the web interface a new primary key
         * for the database is required.
         */
        cyng::vector_t generate_new_key(std::string const &, cyng::vector_t &&key, cyng::param_map_t const &data);

        cyng::data_t complete(std::string const &, cyng::param_map_t const &);

        /**
         * Search a meter record by it's meter id
         */
        cyng::key_t lookup_meter_by_id(std::string const &);

        /**
         * Fill table "meterFull" with data
         */
        void update_table_meter_full();

      private:
        void set_start_values(
            std::uint64_t max_upload_size,
            std::string const &nickname,
            std::chrono::seconds timeout,
            std::string const &country_code,
            std::string const &lang_code);

        /**
         * @return object of the specified type with a generic NULL value
         */
        cyng::object generate_empty_value(cyng::type_code);

      public:
        kv_store cfg_;
        cyng::store &cache_;

      private:
        cyng::logger logger_;
        config::store_map store_map_;

        using array_t = std::array<rel, 21>;
        static array_t const rel_;

        /** @brief channel list
         *
         * Each channel can be subscribed by a multitude of channels
         * channel => ws
         */
        std::multimap<std::string, boost::uuids::uuid> subscriptions_;

        /**
         * generate unique tags
         */
        boost::uuids::random_generator uidgen_;
    };

    /**
     * @return vector of all required meta data
     */
    std::vector<cyng::meta_store> get_store_meta_data();

    cyng::object convert_to_type(cyng::type_code, cyng::object const &);
    cyng::object convert_to_uuid(cyng::object const &);
    cyng::object convert_to_tp(cyng::object const &);
    cyng::object convert_to_ip_address(cyng::object const &);
    cyng::object convert_to_aes128(cyng::object const &);
    cyng::object convert_to_aes192(cyng::object const &obj);
    cyng::object convert_to_aes256(cyng::object const &obj);

    template <typename T> cyng::object convert_to_numeric(cyng::object const &obj) {
        if (obj.tag() == cyng::TC_NULL)
            return cyng::make_object(T(0));
        BOOST_ASSERT(obj.rtti().is_integral());
        return cyng::make_object(cyng::numeric_cast<T>(obj, T(0)));
    }

    cyng::object convert_to_nanoseconds(cyng::object const &);
    cyng::object convert_to_microseconds(cyng::object const &);
    cyng::object convert_to_milliseconds(cyng::object const &);
    cyng::object convert_to_seconds(cyng::object const &);
    cyng::object convert_to_minutes(cyng::object const &);
    cyng::object convert_to_hours(cyng::object const &);

    template <typename T> cyng::object convert_number_to_timespan(cyng::object const &obj) {
        switch (obj.tag()) {
        case cyng::TC_UINT8: return cyng::make_object(T(cyng::numeric_cast<std::uint8_t>(obj, 0)));
        case cyng::TC_UINT16: return cyng::make_object(T(cyng::numeric_cast<std::uint16_t>(obj, 0)));
        case cyng::TC_UINT32: return cyng::make_object(T(cyng::numeric_cast<std::uint32_t>(obj, 0)));
        case cyng::TC_UINT64: return cyng::make_object(T(cyng::numeric_cast<std::uint64_t>(obj, 0)));
        case cyng::TC_INT8: return cyng::make_object(T(cyng::numeric_cast<std::int8_t>(obj, 0)));
        case cyng::TC_INT16: return cyng::make_object(T(cyng::numeric_cast<std::int16_t>(obj, 0)));
        case cyng::TC_INT32: return cyng::make_object(T(cyng::numeric_cast<std::int32_t>(obj, 0)));
        case cyng::TC_INT64: return cyng::make_object(T(cyng::numeric_cast<std::int64_t>(obj, 0)));
        default: break;
        }
        return obj;
    }

} // namespace smf

#endif
