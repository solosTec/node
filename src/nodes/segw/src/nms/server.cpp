/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <nms/session.h>
#include <tasks/nms.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

namespace smf {
    namespace nms {

        server::server(cyng::channel_weak wp, cyng::controller &ctl, cfg &c, cyng::logger logger)
            : state_(state::INITIAL),
              sigs_{
                  std::bind(&server::start, this, std::placeholders::_1), //	0
                  std::bind(&server::stop, this, std::placeholders::_1)   //	1
              },
              cfg_(c),
              logger_(logger),
              acceptor_(ctl.get_ctx()),
              session_counter_{0} {
            auto sp = channel_.lock();
            if (sp) {
                sp->set_channel_name("start", 0);
                CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
            }
        }

        void server::start(boost::asio::ip::tcp::endpoint ep) {
            bool already_openend = false;
            bool already_set_options = false;
            bool already_bind = false;
            bool already_listen = false;
            int sleep_interval_seconds = 2;
            int cnt = 0;
            const int MAX_COUNT = 10;

            boost::system::error_code ec;

            while ((cnt < MAX_COUNT) && (!already_listen)) {
                CYNG_LOG_INFO(logger_, "[NMS] server: loop no.  " << cnt << " of " << MAX_COUNT - 1);
                cnt++;

                if (!already_openend) {
                    acceptor_.open(ep.protocol(), ec);
                    if (ec) {
                        CYNG_LOG_INFO(logger_, "[NMS] server: open failed ")
                        CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds")

                        std::this_thread::sleep_for(std::chrono::seconds(sleep_interval_seconds));
                        sleep_interval_seconds *= 2;
                        continue;
                    } else {
                        already_openend = true;
                        CYNG_LOG_INFO(logger_, "[NMS] server: open sucessful ")
                    }

                } else {
                    CYNG_LOG_INFO(logger_, "[NMS] server: already opened ")
                }

                if (!already_set_options) {
                    acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
                    if (ec) {
                        CYNG_LOG_INFO(logger_, "[NMS] server: set option failed ")
                        CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds");
                        std::this_thread::sleep_for(std::chrono::seconds(sleep_interval_seconds));
                        sleep_interval_seconds *= 2;
                        continue;
                    } else {
                        already_set_options = true;
                        CYNG_LOG_INFO(logger_, "[NMS] server: set options sucessful ")
                    }
                }
                if (!already_bind) {
                    acceptor_.bind(ep, ec);
                    if (ec) {
                        CYNG_LOG_INFO(logger_, "[NMS] server: bind failed ")
                        CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds");
                        std::this_thread::sleep_for(std::chrono::seconds(sleep_interval_seconds));
                        sleep_interval_seconds *= 2;
                        continue;
                    } else {
                        already_bind = true;
                        CYNG_LOG_INFO(logger_, "[NMS] server: bind sucessful ")
                    }
                }

                if (!already_listen) {
                    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
                    if (ec) {
                        CYNG_LOG_INFO(logger_, "[NMS] server: listen failed ")
                        CYNG_LOG_INFO(logger_, "[NMS] server: going to sleep  " << sleep_interval_seconds << " seconds");
                        std::this_thread::sleep_for(std::chrono::seconds(sleep_interval_seconds));
                        sleep_interval_seconds *= 2;
                        continue;
                    } else {
                        already_listen = true;
                        CYNG_LOG_INFO(logger_, "[NMS] server: listen sucessful ")
                    }
                }
            }

            if (already_listen) {
                do_accept();
            } else {
                CYNG_LOG_WARNING(logger_, "[NMS] server cannot start listening at " << ep << ": " << ec.message());
            }
        }

        void server::do_accept() {
            acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    CYNG_LOG_INFO(logger_, "[NMS] new session " << socket.remote_endpoint());

                    auto sp = std::shared_ptr<session>(
                        new session(std::move(socket), cfg_, logger_, std::bind(&server::rebind, this, std::placeholders::_1)),
                        [this](session *s) {
                            //
                            //	update session counter
                            //
                            --session_counter_;
                            CYNG_LOG_TRACE(logger_, "[NMS] session(s) running: " << session_counter_);

                            //
                            //	remove session
                            //
                            delete s;
                        });

                    if (sp) {

                        //
                        //	start session
                        //
                        sp->start();

                        //
                        //	update session counter
                        //
                        ++session_counter_;
                    }

                    //
                    //	continue listening
                    //
                    do_accept();
                } else {
                    CYNG_LOG_WARNING(logger_, "[NMS] server stopped: " << ec.message());
                }
            });
        }

        void server::stop() {
            boost::system::error_code ec;
            acceptor_.cancel(ec);
            acceptor_.close(ec);
            if (session_counter_ != 0) {
                CYNG_LOG_WARNING(logger_, "[NMS] server " << session_counter_ << " session(s) still running");
            }
        }

        void server::rebind(boost::asio::ip::tcp::endpoint ep) {

            acceptor_ = boost::asio::ip::tcp::acceptor(acceptor_.get_executor(), ep);
        }

    } // namespace nms
} // namespace smf
