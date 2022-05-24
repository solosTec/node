/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CFG_H
#define SMF_SEGW_CFG_H

#include <smf/config/kv_store.h>
#include <smf/sml/status.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/value_cast.hpp>
#include <cyng/store/db.h>

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/uuid.hpp>

namespace smf {

    class bridge;
    class counter;

    /**
     * manage configuration data
     */
    class cfg : public kv_store {
        friend class bridge;
        friend class counter; //  manage operation time counter

      public:
        cfg(cyng::logger, cyng::store &, std::tuple<boost::uuids::uuid, cyng::buffer_t, std::uint32_t>);

        inline cyng::store &get_cache() { return cache_; }

        /**
         * @return generate an UUID using the SHA1 hashing algorithm
         */
        boost::uuids::uuid get_name(std::string const &) const;
        boost::uuids::uuid get_name(cyng::buffer_t const &) const;

        /**
         * get configured server ID (OBIS_SERVER_ID:)
         * 05 + MAC
         */
        cyng::buffer_t get_srv_id() const;

        /**
         * @return SML status word
         */
        sml::status_word_t get_status_word() const;

        /**
         * update status word
         */
        void update_status_word(sml::status_bit, bool);

        /**
         * @return Operating hours counter (in seconds)
         */
        std::uint32_t get_operating_time() const;

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
         * server id
         * 05 + MAC
         * initialized in bridge::load_configuration()
         */
        cyng::buffer_t const id_;

        /**
         * OBIS log status
         */
        sml::status_word_t status_word_;

        boost::uuids::name_generator_sha1 name_gen_;

        /**
         * operating time in seconds
         */
        std::uint32_t op_time_;
    };
} // namespace smf

#endif
