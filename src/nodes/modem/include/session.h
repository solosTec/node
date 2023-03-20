/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODEM_SESSION_H
#define SMF_MODEM_SESSION_H

#include <smf/session/session.hpp>

#include <smf/modem/parser.h>
#include <smf/modem/serializer.h>

namespace smf {

    class modem_session : public session<modem::parser, modem::serializer, modem_session, 2048> {

        //  base class
        using base_t = session<modem::parser, modem::serializer, modem_session, 2048>;

      public:
        modem_session(
            boost::asio::ip::tcp::socket socket,
            bus &cluster_bus,
            cyng::mesh &fabric,
            cyng::logger logger,
            bool auto_answer,
            std::chrono::milliseconds guard);
        virtual ~modem_session();

        void stop();
        void logout();

      private:
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
        bool const auto_answer_;
    };

} // namespace smf

#endif
