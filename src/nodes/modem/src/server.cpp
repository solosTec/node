#include <server.h>
#include <session.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/controller.h>

#include <iostream>

namespace smf {

    modem_server::modem_server(
        boost::asio::io_context &ioc,
        cyng::logger logger,
        bool auto_answer,
        std::chrono::milliseconds guard,
        std::chrono::seconds timeout, //  gatekeeper
        bus &cluster_bus,
        cyng::mesh &fabric)
        : logger_(logger)
        , auto_answer_(auto_answer)
        , guard_(guard)
        , timeout_(timeout)
        , cluster_bus_(cluster_bus)
        , fabric_(fabric)
        , acceptor_(ioc)
        , session_counter_{0} {}

    modem_server::~modem_server() {
#ifdef _DEBUG_MODEM
        std::cout << "modem(~)" << std::endl;
#endif
    }

    void modem_server::listen(boost::asio::ip::tcp::endpoint ep) {

        boost::system::error_code ec;
        acceptor_.open(ep.protocol(), ec);
        if (!ec)
            acceptor_.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
        if (!ec)
            acceptor_.bind(ep, ec);
        if (!ec)
            acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (!ec) {
            CYNG_LOG_INFO(logger_, "server starts listening at " << ep);
            do_accept();
        } else {
            CYNG_LOG_WARNING(logger_, "server cannot start listening at " << ep << ": " << ec.message());
        }
    }

    void modem_server::do_accept() {

        acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                CYNG_LOG_INFO(logger_, "new session " << socket.remote_endpoint());

                auto sp = std::shared_ptr<modem_session>(
                    new modem_session(std::move(socket), cluster_bus_, fabric_, auto_answer_, guard_, logger_),
                    [this](modem_session *s) {
                        //
                        //	update cluster state
                        //
                        s->logout();
                        s->stop();

                        //
                        //	update session counter
                        //
                        --session_counter_;
                        CYNG_LOG_TRACE(logger_, "session(s) running: " << session_counter_);

                        //
                        //	remove session
                        //
                        delete s;
                    });

                if (sp) {

                    //
                    //	start session
                    //
                    sp->start(timeout_);

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
                CYNG_LOG_WARNING(logger_, "server stopped: " << ec.message());
            }
        });
    }

} // namespace smf
