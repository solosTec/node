/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_BROKER_IEC_DB_H
#define SMF_BROKER_IEC_DB_H

#include <smf/config/kv_store.h>
#include <smf/config/schemes.h>

#include <cyng/log/logger.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/store/db.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

    class db : public cyng::slot_interface, public std::enable_shared_from_this<db> {
      public:
        db(cyng::store &cache, cyng::logger, boost::uuids::uuid tag, cyng::channel_weak channel);

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

        /**
         * loop over all table meta data
         */
        std::string get_first_table() const;

        /**
         * get next table name from get_store_meta_data()
         *
         * @return empty string if no more tables available
         */
        std::string get_next_table(std::string const &) const;

        cyng::meta_store get_meta(std::string) const;

      public:
        cyng::store &cache_;

      private:
        cyng::logger logger_;
        cyng::channel_weak channel_;
        config::store_map store_map_;
    };

    std::vector<cyng::meta_store> get_store_meta_data();

    /**
     * helper class to store temporary config data
     */
    struct sub_meter {
        std::string id_;
        cyng::key_t key_;
        sub_meter(std::string const &, cyng::key_t const &);
    };
} // namespace smf

#endif
