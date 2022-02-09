/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_BROKER_WMBUS_DB_H
#define SMF_BROKER_WMBUS_DB_H

#include <smf/config/kv_store.h>
#include <smf/config/schemes.h>

#include <cyng/log/logger.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/store/db.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

    class bus;
    class db : public cyng::slot_interface {
      public:
        db(cyng::store &cache, cyng::logger, boost::uuids::uuid tag);

        void res_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag);

        void res_update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag);

        void res_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag);

        void res_clear(std::string table_name, boost::uuids::uuid tag);

        void init(cyng::slot_ptr slot);
        void disconnect(cyng::slot_ptr slot);

        /**
         * insert
         */
        virtual bool
        forward(cyng::table const *, cyng::key_t const &, cyng::data_t const &, std::uint64_t, boost::uuids::uuid) override;

        /**
         * update
         */
        virtual bool forward(
            cyng::table const *tbl,
            cyng::key_t const &key,
            cyng::attr_t const &attr,
            cyng::data_t const &data,
            std::uint64_t gen,
            boost::uuids::uuid tag) override;

        /**
         * remove
         */
        virtual bool
        forward(cyng::table const *tbl, cyng::key_t const &key, cyng::data_t const &data, boost::uuids::uuid tag) override;

        /**
         * clear
         */
        virtual bool forward(cyng::table const *, boost::uuids::uuid) override;

        /**
         * transaction
         */
        virtual bool forward(cyng::table const *, bool) override;

        std::string get_first_table() const;
        /**
         * get next table name from get_store_meta_data()
         *
         * @return empty string if no more tables available
         */
        std::string get_next_table(std::string const &) const;

        /**
         * uuid is nil if meter wasn't found
         */
        std::pair<cyng::crypto::aes_128_key, boost::uuids::uuid> lookup_meter(std::string);

        cyng::key_t update_gw_status(boost::asio::ip::tcp::endpoint, bus &);

      public:
        cyng::store &cache_;
        boost::uuids::uuid const tag_;

      private:
        cyng::logger logger_;
        config::store_map store_map_;
        config::dependend_key dep_key_;
    };

    std::vector<cyng::meta_store> get_store_meta_data();
} // namespace smf

#endif
