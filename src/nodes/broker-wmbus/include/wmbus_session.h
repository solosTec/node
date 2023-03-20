/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_WMBUS_SESSION_H
#define SMF_WMBUS_SESSION_H

#include <db.h>

#include <smf/mbus/radio/parser.h>
#include <smf/session/session.hpp>

#include <cyng/obj/intrinsics/pid.h>

namespace smf {
    namespace mbus {
        //  dummy class
        class serializer {};
    } // namespace mbus

    class wmbus_session : public session<mbus::radio::parser, mbus::serializer, wmbus_session, 2048> {

        //  base class
        using base_t = session<mbus::radio::parser, mbus::serializer, wmbus_session, 2048>;

      public:
        wmbus_session(
            boost::asio::ip::tcp::socket socket,
            bus &cluster_bus,
            cyng::mesh &fabric,
            cyng::logger,
            std::shared_ptr<db>,
            cyng::key_t,
            cyng::channel_ptr writer);
        virtual ~wmbus_session();

        void stop();

      private:
        void decode(mbus::radio::header const &h, mbus::radio::tplayer const &t, cyng::buffer_t const &data);
        void decode(srv_id_t id, std::uint8_t access_no, std::uint8_t frame_type, cyng::buffer_t const &data);

        void push_sml_data(cyng::buffer_t const &payload);
        void push_dlsm_data(cyng::buffer_t const &payload);
        void push_data(cyng::buffer_t const &payload);

        std::size_t read_mbus(
            srv_id_t const &address,
            std::string const &id,
            std::uint8_t medium,
            std::string const &manufacturer,
            std::uint8_t frame_type,
            cyng::buffer_t const &payload);

        /**
         * @return number of data records
         */
        std::size_t read_sml(srv_id_t const &address, cyng::buffer_t const &payload);

      private:
        std::shared_ptr<db> db_;
        cyng::key_t const key_gw_wmbus_;
        cyng::channel_ptr writer_;
    };

} // namespace smf

#endif
