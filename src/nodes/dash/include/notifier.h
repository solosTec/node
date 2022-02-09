/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_NOTIFIER_H
#define SMF_DASH_NOTIFIER_H

#include <cyng/log/logger.h>
#include <cyng/store/slot_interface.h>

namespace cyng {
    class record;
}

namespace smf {

    class db;
    class http_server;
    class notifier : public cyng::slot_interface {

        enum class event_type { INSERT, UPDATE, REMOVE };

      public:
        notifier(db &data, http_server &, cyng::logger);

      public:
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

      private:
        void update_meter_online_state(cyng::record const &, event_type);
        void update_gw_online_state(cyng::record const &, event_type);
        void update_meter_iec_online_state(cyng::record const &, event_type);
        void update_meter_wmbus_online_state(cyng::record const &, event_type);

        static std::string get_name(event_type);
        static std::uint32_t calc_connection_state(event_type evt, cyng::record const &);

      private:
        db &db_;
        http_server &http_server_;
        cyng::logger logger_;
    };

} // namespace smf

#endif
