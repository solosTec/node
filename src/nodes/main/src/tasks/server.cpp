/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <session.h>
#include <tasks/server.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    server::server(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& country_code
		, std::string const& lang_code
		, cyng::logger logger
		, std::string const& account
		, std::string const& pwd
		, std::string const& salt
		, std::chrono::seconds monitor
		, cyng::param_map_t const& session_cfg)
	: sigs_{ 
		std::bind(&server::start, this, std::placeholders::_1),
		std::bind(&server::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, tag_(tag)
		, logger_(logger)
		, account_(account)
		, pwd_(pwd)
		, salt_(salt)
		, monitor_(monitor)
		, acceptor_(ctl.get_ctx())
		, session_counter_{ 0 }
		, fabric_(ctl)
		, store_()
		, cache_(store_, logger, tag)
	{
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_name("start", 0);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }

        //
        //	create all tables and set initial values
        //
        cache_.init(session_cfg, country_code, lang_code);
    }

    server::~server() {
#ifdef _DEBUG_MAIN
        std::cout << "server(~)" << std::endl;
#endif
    }

    void server::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")"); }

    void server::start(boost::asio::ip::tcp::endpoint ep) {
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

    void server::do_accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                auto const ep = socket.remote_endpoint();
                CYNG_LOG_INFO(logger_, "new session " << ep);

                auto sp = std::shared_ptr<session>(new session(std::move(socket), cache_, fabric_, logger_), [this](session *s) {
                    BOOST_ASSERT(s != nullptr);

                    //
                    //  stop ping
                    //
                    // pt->stop();

                    //
                    //	remove from cluster table
                    //
                    auto const ptys = cache_.remove_pty_by_peer(s->get_peer(), s->get_remote_peer());
                    CYNG_LOG_TRACE(logger_, "session [" << s->get_peer() << "] with " << ptys << " users closed");

                    //
                    //	stop session and
                    //	remove all subscriptions
                    //
                    s->stop();

                    //
                    //	update session counter
                    //
                    BOOST_ASSERT(session_counter_ != 0);
                    --session_counter_;
                    CYNG_LOG_TRACE(logger_, "session(s) running: " << session_counter_);
                    cache_.sys_msg(cyng::severity::LEVEL_TRACE, session_counter_, "node(s) online - ", ptys);

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
                    cache_.sys_msg(cyng::severity::LEVEL_TRACE, session_counter_, "node(s) online + ", ep);
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
