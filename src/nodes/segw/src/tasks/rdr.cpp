/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <redirector/session.h>
#include <tasks/rdr.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

namespace smf {
    namespace rdr {

        server::server(cyng::channel_weak wp, cyng::controller &ctl, cfg &c, cyng::logger logger, lmn_type type, server::type srv_type, boost::asio::ip::tcp::endpoint ep)
            : sigs_{
                  std::bind(&server::start, this, std::placeholders::_1), //	0
                  std::bind(&server::pause, this), //	1
                  std::bind(&server::stop, this, std::placeholders::_1)   //	2
              }
            , channel_(wp)
            , srv_type_(srv_type)
            , ctl_(ctl)
            , cfg_(c)
            , logger_(logger)
            , type_(type)
            , ep_(ep)
            , registry_(ctl.get_registry())
            , acceptor_(ctl.get_ctx())
            , session_counter_{0} {
            auto sp = channel_.lock();
            if (sp) {
                std::size_t slot{0};
                sp->set_channel_name("start", slot++);
                sp->set_channel_name("halt", slot++);
                CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
            }
        }

        void server::start(std::chrono::seconds delay) {
            boost::system::error_code ec;
            acceptor_.open(ep_.protocol());
            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[RDR] acceptor open: " << ep_);
                acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
            }
            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[RDR] reuse address");
                acceptor_.bind(ep_, ec);
            }
            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[RDR] bind: " << ep_);
                acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
            }
            if (!ec) {
                CYNG_LOG_INFO(logger_, "[RDR] " << get_name(type_) << " starts listening at " << ep_);
                do_accept();
            } else {
                CYNG_LOG_WARNING(logger_, "[RDR] " << get_name(type_) << " server cannot start listening at " << ep_ << ": " << ec.message());
                //
                //  reset acceptor
                //
                acceptor_.cancel(ec);
                acceptor_.close(ec);

                auto sp = channel_.lock();
                if (sp)
                    sp->suspend(delay, "start", cyng::make_tuple(delay));
            }
        }

        void server::do_accept() {
            acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    CYNG_LOG_INFO(logger_, "[RDR] " << get_name(type_) << " new session " << socket.remote_endpoint());

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
                            auto sp = channel_.lock();
                            if (sp && session_counter_ == 0) {
                                cfg_listener cfg(cfg_, type_);
                                auto const delay = cfg.get_delay();
                                CYNG_LOG_INFO(logger_, "[RDR] restart other listener");
                                auto const count = ctl_.get_registry().dispatch_exclude(sp, "start", cyng::make_tuple(delay));
                                if (count == 0) {
                                    CYNG_LOG_WARNING(logger_, "[RDR] no other listener available");
                                }
                                else {
                                    CYNG_LOG_TRACE(logger_, "[RDR] #" << count << " other listener(s) restarted");
                                }
                            }
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
                        auto sp = channel_.lock();
                        if (sp && session_counter_ == 1) {
                            CYNG_LOG_WARNING(logger_, "[RDR] halt other listener than " << sp->get_id());
                            auto const count = ctl_.get_registry().dispatch_exclude(sp, "halt", cyng::make_tuple());
                            if (count == 0) {
                                CYNG_LOG_WARNING(logger_, "[RDR] no other listener available");
                            }
                            else {
                                CYNG_LOG_TRACE(logger_, "[RDR] #" << count << " other listener(s) halted");
                            }
                        }
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

        void server::stop(cyng::eod) {
            boost::system::error_code ec;
            acceptor_.cancel(ec);
            acceptor_.close(ec);
        }

        void server::pause() {
            CYNG_LOG_WARNING(logger_, "[RDR] server paused");
            stop(cyng::eod());
        }

    } // namespace rdr
} // namespace smf
