/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_IMEGA_SESSION_H
#define SMF_IMEGA_SESSION_H

#include <smf/session/session.hpp>

#include <smf/imega.h>
#include <smf/imega/parser.h>
#include <smf/imega/serializer.h>

namespace smf {

    class imega_session : public session<imega::parser, imega::serializer, imega_session, 2048> {

        //  base class
        using base_t = session<imega::parser, imega::serializer, imega_session, 2048>;

      public:
        imega_session(
            boost::asio::ip::tcp::socket socket,
            bus &cluster_bus,
            cyng::mesh &fabric,
            cyng::logger logger,
            imega::policy policy,
            std::string const &pwd);

        virtual ~imega_session();

        void stop();
        void logout();

      private:
        cyng::vm_proxy init_vm(cyng::mesh &fabric);

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
    };

} // namespace smf

#endif
