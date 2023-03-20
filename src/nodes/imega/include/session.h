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

        // void start(std::chrono::seconds timeout);
        void stop();
        void logout();

        boost::asio::ip::tcp::endpoint get_remote_endpoint() const;

      private:
        // void do_read();
        // void do_write();
        // void handle_write(const boost::system::error_code &ec);

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

        /**
         * send data to device
         */
        // void imega_send(std::function<cyng::buffer_t()> f);

      private:
        // cyng::controller &ctl_;
        // boost::asio::ip::tcp::socket socket_;
        // cyng::logger logger_;

        // bus &cluster_bus_;

        /**
         * Buffer for incoming data.
         */
        // std::array<char, 2048> buffer_;
        // std::uint64_t rx_, sx_;

        /**
         * Buffer for outgoing data.
         */
        // std::deque<cyng::buffer_t> buffer_write_;

        /**
         * parser for imega data
         */
        // imega::parser parser_;

        /**
         * serializer for imega data
         */
        // imega::serializer serializer_;

        /**
         * client VM
         */
        // cyng::vm_proxy vm_;

        /**
         * tag/pk of device
         */
        // boost::uuids::uuid dev_;

        /**
         * gatekeeper
         */
        // cyng::channel_ptr gatekeeper_;
    };

} // namespace smf

#endif
