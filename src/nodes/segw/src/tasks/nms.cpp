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
            : sigs_{
                  std::bind(&server::start, this, std::placeholders::_1), //	0
                  std::bind(&server::stop, this, std::placeholders::_1)   //	1
              },
              channel_(wp),
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

            boost::system::error_code ec;
            acceptor_.open(ep.protocol(), ec);
            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[NMS] acceptor open: " << ep);
                acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
            }
            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[NMS] reuse address");
                acceptor_.bind(ep, ec);
            }
            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[NMS] bind: " << ep);
                acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
            }
            if (!ec) {
                CYNG_LOG_INFO(logger_, "[NMS] starts listening at: " << ep);
                do_accept();
            } else {
                CYNG_LOG_WARNING(logger_, "[NMS] server cannot start listening at " << ep << ": " << ec.message());

                //
                //  reset acceptor
                //
                acceptor_.cancel(ec);
                acceptor_.close(ec);

                //
                //  retry
                //
                auto sp = channel_.lock();
                if (sp)
                    sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple(ep));
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
                } else if (ec != boost::asio::error::operation_aborted) {
                    CYNG_LOG_WARNING(logger_, "[NMS] server stopped: " << ec.message());
                }
            });
        }

        void server::stop(cyng::eod) {
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
