/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODEM_SESSION_H
#define SMF_MODEM_SESSION_H

#include <smf/cluster/bus.h>
#include <smf/modem/parser.h>
#include <smf/modem/serializer.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/task/controller.h>
#include <cyng/vm/proxy.h>
#include <cyng/vm/vm_fwd.h>

#include <array>
#include <memory>

namespace smf {

    class modem_session : public std::enable_shared_from_this<modem_session> {
      public:
        modem_session(
            boost::asio::ip::tcp::socket socket,
            bus &cluster_bus,
            cyng::mesh &fabric,
            bool auto_answer,
            std::chrono::milliseconds guard,
            cyng::logger logger);
        ~modem_session();

        void start(std::chrono::seconds timeout);
        void stop();
        void logout();

        boost::asio::ip::tcp::endpoint get_remote_endpoint() const;

      private:
        void do_read();
        void do_write();
        void handle_write(const boost::system::error_code &ec);
        void print(cyng::buffer_t &&);

        void pty_stop();

        void cfg_backup(
            std::string,
            std::string,
            boost::uuids::uuid,
            cyng::buffer_t,
            std::string,
            std::chrono::system_clock::time_point tp);

        //
        //	bus interface
        //
        void pty_res_login(bool success, boost::uuids::uuid dev);

        void pty_res_open_connection(bool success, cyng::param_map_t token);

        void pty_transfer_data(cyng::buffer_t data);

        void pty_res_close_connection(bool success, cyng::param_map_t token);

        void pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token);

        void pty_req_close_connection();

      private:
        cyng::controller &ctl_;
        boost::asio::ip::tcp::socket socket_;
        bool const auto_answer_;
        cyng::logger logger_;

        bus &cluster_bus_;

        /**
         * Buffer for incoming data.
         */
        std::array<char, 2048> buffer_;
        std::uint64_t rx_, sx_, px_;

        /**
         * Buffer for outgoing data.
         */
        std::deque<cyng::buffer_t> buffer_write_;

        /**
         * parser for modem data
         */
        modem::parser parser_;

        /**
         * serializer for modem data
         */
        modem::serializer serializer_;

        cyng::vm_proxy vm_;

        /**
         * tag/pk of device
         */
        boost::uuids::uuid dev_;

        /**
         * gatekeeper
         */
        cyng::channel_ptr gatekeeper_;
    };

} // namespace smf

#endif
