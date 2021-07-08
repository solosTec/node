/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_WMBUS_SESSION_H
#define SMF_WMBUS_SESSION_H

#include <db.h>
#include <smf/cluster/bus.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/io/parser/parser.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/pid.h>
#include <cyng/task/controller.h>
#include <cyng/vm/proxy.h>

#include <array>
#include <memory>

namespace smf {

    class wmbus_session : public std::enable_shared_from_this<wmbus_session> {
      public:
        wmbus_session(
            cyng::controller &ctl,
            boost::asio::ip::tcp::socket socket,
            std::shared_ptr<db>,
            cyng::logger,
            bus &,
            cyng::key_t,
            cyng::channel_ptr writer);
        ~wmbus_session();

        void start(std::chrono::seconds timeout);
        void stop();

      private:
        void do_read();
        void do_write();
        void handle_write(const boost::system::error_code &ec);

        void decode(mbus::radio::header const &h, mbus::radio::tpl const &t, cyng::buffer_t const &data);
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
        cyng::controller &ctl_;
        boost::asio::ip::tcp::socket socket_;
        std::shared_ptr<db> db_;
        cyng::logger logger_;
        bus &bus_;
        cyng::key_t const key_gw_wmbus_;
        cyng::channel_ptr writer_;

        /**
         * Buffer for incoming data.
         */
        std::array<char, 2048> buffer_;

        /**
         * Buffer for outgoing data.
         */
        std::deque<cyng::buffer_t> buffer_write_;

        /**
         * parser for wireless M-Bus data
         */
        mbus::radio::parser parser_;

        /**
         * gatekeeper
         */
        cyng::channel_ptr gatekeeper_;
    };

} // namespace smf

#endif
