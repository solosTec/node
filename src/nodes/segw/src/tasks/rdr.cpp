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

        server::server(cyng::channel_weak wp, cyng::controller &ctl, cfg &c, cyng::logger logger, lmn_type type, server::type srv_type)
            : sigs_{
                  std::bind(&server::start, this, std::placeholders::_1), //	0
                  std::bind(&server::pause, this), //	1 (halt)
                  std::bind(&server::restart, this), //	2
                  std::bind(&server::stop, this, std::placeholders::_1)   //	3
              }
            , channel_(wp)
            , srv_type_(srv_type)
            , ctl_(ctl)
            , cfg_(c)
            , logger_(logger)
            , type_(type)
            , registry_(ctl.get_registry())
            , acceptor_(ctl.get_ctx())
            , session_counter_{0} {
            auto sp = channel_.lock();
            if (sp) {
                std::size_t slot{0};
                sp->set_channel_name("start", slot++);
                sp->set_channel_name("halt", slot++);
                sp->set_channel_name("restart", slot++);
                CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
            }
        }

#ifdef _DEBUG
        server::~server() { CYNG_LOG_TRACE(logger_, "[RDR] server::~server()"); }
#endif

        void server::start(std::chrono::seconds delay) {
            auto sp = channel_.lock();
            BOOST_ASSERT(sp);
            if (sp) {

                //
                //  get TPC/IP endpoint
                //
                auto const ep = get_endpoint();
                if (!ep.address().is_unspecified()) {
                    CYNG_LOG_INFO(logger_, "[RDR] #" << sp->get_id() << " start at endpoint [" << ep << "]");
                } else {
                    CYNG_LOG_WARNING(logger_, "[RDR] #" << sp->get_id() << " endpoint [" << ep << "] is not present");
                }

                boost::system::error_code ec;
                acceptor_.open(ep.protocol(), ec);
                if (!ec) {
                    CYNG_LOG_TRACE(logger_, "[RDR] #" << sp->get_id() << " acceptor open: " << ep);
                    acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
                }
                if (!ec) {
                    CYNG_LOG_TRACE(logger_, "[RDR] #" << sp->get_id() << " reuse address");
                    acceptor_.bind(ep, ec);
                }
                if (!ec) {
                    CYNG_LOG_TRACE(logger_, "[RDR] #" << sp->get_id() << " bind: " << ep);
                    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
                }
                if (!ec) {
                    CYNG_LOG_INFO(logger_, "[RDR] #" << sp->get_id() << " " << get_name(type_) << " starts listening at " << ep);
                    do_accept();
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "[RDR] #" << sp->get_id() << " " << get_name(type_) << " server cannot start listening at " << ep << ": "
                                  << ec.message());
                    //
                    //  reset acceptor
                    //
                    acceptor_.cancel(ec);
                    acceptor_.close(ec);

                    sp->suspend(delay, "start", cyng::make_tuple(delay));
                }
            } else {
                CYNG_LOG_FATAL(logger_, "[RDR] task removed");
            }
        }

        boost::asio::ip::tcp::endpoint server::get_endpoint() {
            cfg_listener cfg(cfg_, type_);
            return (srv_type_ == type::ipv6) ? cfg.get_link_local_ep() : cfg.get_ipv4_ep();
        }

        void server::do_accept() {
            auto cp = channel_.lock();
            BOOST_ASSERT(cp);
            acceptor_.async_accept([this, cp](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    CYNG_LOG_INFO(
                        logger_,
                        "[RDR] #" << cp->get_id() << " " << get_name(type_) << " new session " << socket.remote_endpoint());

                    auto sp = std::shared_ptr<session>(
                        new session(ctl_.get_ctx(), std::move(socket), registry_, cfg_, logger_, type_), [this, cp](session *s) {
                            //
                            //  stop deadline timer
                            //
                            s->stop_timer();

                            //
                            //	update session counter
                            //
                            --session_counter_;
                            CYNG_LOG_TRACE(logger_, "[RDR] #" << cp->get_id() << " " << session_counter_ << " session(s) running");

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
                                CYNG_LOG_INFO(logger_, "[RDR] #" << cp->get_id() << " restart other listener");
                                auto const count = ctl_.get_registry().dispatch_exclude(sp, "start", cyng::make_tuple(delay));
                                if (count == 0) {
                                    CYNG_LOG_WARNING(logger_, "[RDR] #" << cp->get_id() << " no other listener available");
                                } else {
                                    CYNG_LOG_TRACE(
                                        logger_, "[RDR] #" << cp->get_id() << " " << count << " other listener(s) restarted");
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
                        auto cp = channel_.lock();
                        if (cp && session_counter_ == 1) {
                            CYNG_LOG_WARNING(logger_, "[RDR] #" << cp->get_id() << " halt other listener(s)");
                            auto const count = ctl_.get_registry().dispatch_exclude(cp, "halt", cyng::make_tuple());
                            if (count == 0) {
                                CYNG_LOG_WARNING(logger_, "[RDR] #" << cp->get_id() << " no other listeners available");
                            } else {
                                CYNG_LOG_TRACE(logger_, "[RDR] #" << cp->get_id() << " " << count << " other listener(s) halted");
                            }
                        }
                    }

                    //
                    //	continue listening
                    //
                    do_accept();
                } else if (ec != boost::asio::error::operation_aborted) {
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
            auto sp = channel_.lock();
            BOOST_ASSERT(sp);
            CYNG_LOG_WARNING(logger_, "[RDR] #" << sp->get_id() << " server paused");
            stop(cyng::eod());
        }

        void server::restart() {

            auto sp = channel_.lock();
            BOOST_ASSERT(sp);
            cfg_listener cfg(cfg_, type_);
            CYNG_LOG_INFO(logger_, "[RDR] #" << sp->get_id() << " restart");

            if (acceptor_.is_open()) {

                boost::system::error_code ec;
                acceptor_.cancel(ec);
                acceptor_.close(ec);

            } else {
                CYNG_LOG_WARNING(logger_, "[RDR]] #" << sp->get_id() << " listener (still) closed - cannot change endpoint");
            }

            //
            //  restart
            //
            auto const delay = cfg.get_delay();
            start(delay);
        }

    } // namespace rdr
} // namespace smf
