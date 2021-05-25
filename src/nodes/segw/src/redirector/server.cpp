/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <redirector/server.h>
#include <redirector/session.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

namespace smf {
    namespace rdr {

        server::server(cyng::controller &ctl, cfg &c, cyng::logger logger, lmn_type type, server::type srv_type)
            : srv_type_(srv_type)
            , ctl_(ctl)
            , cfg_(c)
            , logger_(logger)
            , type_(type)
            , registry_(ctl.get_registry())
            , acceptor_(ctl.get_ctx())
            , session_counter_{0} {}

        void server::start(boost::asio::ip::tcp::endpoint ep) {
            boost::system::error_code ec;
            acceptor_.open(ep.protocol());
            if (!ec)
                acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
            if (!ec)
                acceptor_.bind(ep, ec);
            if (!ec)
                acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
            if (!ec) {
                acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
                CYNG_LOG_TRACE(logger_, "[RDR] starts listening at " << ep);
                do_accept();
            } else {
                CYNG_LOG_WARNING(logger_, "[RDR] server cannot start listening at " << ep << ": " << ec.message());
            }
        }

        void server::do_accept() {
            acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    CYNG_LOG_INFO(logger_, "[RDR] new session " << socket.remote_endpoint());

                    auto sp = std::shared_ptr<session>(
                        new session(std::move(socket), registry_, cfg_, logger_, type_), [this](session *s) {
                            //
                            //	update session counter
                            //
                            --session_counter_;
                            CYNG_LOG_TRACE(logger_, "[RDR] session(s) running: " << session_counter_);

                            //
                            //	remove session
                            //
                            delete s;

                            //
                            //  stop/start other server of the same type
                            //
                            ctl_.get_registry().dispatch(
                                "bridge", "rdr.activity", session_counter_, get_index(type_), static_cast<std::uint8_t>(srv_type_));
                        });

                    if (sp) {

                        //
                        //	start session
                        //
                        sp->start(ctl_);

                        //
                        //	update session counter
                        //
                        ++session_counter_;

                        //
                        //  stop/start other server of the same type
                        //
                        ctl_.get_registry().dispatch(
                            "bridge", "rdr.activity", session_counter_, get_index(type_), static_cast<std::uint8_t>(srv_type_));
                    }

                    //
                    //	continue listening
                    //
                    do_accept();
                } else {
                    CYNG_LOG_WARNING(logger_, "[RDR] server stopped: " << ec.message());
                }
            });
        }
        void server::stop() {
            boost::system::error_code ec;
            acceptor_.cancel(ec);
            acceptor_.close(ec);
        }

    } // namespace rdr
} // namespace smf
