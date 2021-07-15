/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <redirector/session.h>
#include <tasks/forwarder.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>
#include <cyng/task/registry.h>

#include <boost/bind.hpp>

namespace smf {
    namespace rdr {

        session::session(
            boost::asio::io_context &ctx,
            boost::asio::ip::tcp::socket socket,
            cyng::registry &reg,
            cfg &config,
            cyng::logger logger,
            lmn_type type)
            : socket_(std::move(socket))
            , registry_(reg)
            , logger_(logger)
            , cfg_(config, type)
            , idle_timer_(ctx)
            , buffer_{0}
            , channel_() {}

        void session::start(cyng::controller &ctl) {

            //
            //	start receiver task
            //
            channel_ = ctl.create_named_channel_with_ref<forwarder>(
                "redirector", registry_, logger_, std::bind(&session::do_write, this->shared_from_this(), std::placeholders::_1));
            BOOST_ASSERT(channel_->is_open());

            //
            //	port name == LMN task name
            //
            channel_->dispatch("connect", cyng::make_tuple(cfg_.get_port_name()));

            //
            //	start reading incoming data (mostly "/?!")
            //
            do_read();

            CYNG_LOG_TRACE(logger_, "[RDR] start deadline timer: " << cfg_.get_timeout().total_seconds() << " seconds");
            idle_timer_.expires_from_now(cfg_.get_timeout());
            idle_timer_.async_wait(boost::bind(&session::timeout, this->shared_from_this(), boost::asio::placeholders::error));
        }

        void session::timeout(boost::system::error_code ec) {
            if (!ec) {
                // Timer has expired, but the read operation's completion handler
                // may have already ran, setting expiration to be in the future.
                if (idle_timer_.expires_at() > boost::asio::deadline_timer::traits_type::now()) {
                    //
                    //  restart timer
                    //
                    CYNG_LOG_TRACE(logger_, "[RDR] restart idle timer: " << cfg_.get_timeout().total_seconds() << " seconds");
                    idle_timer_.expires_from_now(cfg_.get_timeout());
                    idle_timer_.async_wait(
                        boost::bind(&session::timeout, this->shared_from_this(), boost::asio::placeholders::error));
                } else {
                    //
                    //  timeout - close socket
                    //
                    CYNG_LOG_WARNING(logger_, "[RDR] timeout");
                    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                    socket_.close(ec);
                }
            }
        }

        void session::do_write(cyng::buffer_t data) {
            //
            //  reset idle timer
            //
            idle_timer_.expires_from_now(cfg_.get_timeout());
            idle_timer_.async_wait(boost::bind(&session::timeout, this->shared_from_this(), boost::asio::placeholders::error));

            //
            //	write it straight to the socket
            //
            boost::system::error_code ec;
            boost::asio::write(socket_, boost::asio::buffer(data.data(), data.size()), ec);
        }

        void session::do_read() {

            auto self = shared_from_this();

            socket_.async_read_some(
                boost::asio::buffer(buffer_.data(), buffer_.size()),
                [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                    if (!ec) {

                        //
                        //  reset idle timer
                        //
                        idle_timer_.expires_from_now(cfg_.get_timeout());
                        idle_timer_.async_wait(
                            boost::bind(&session::timeout, this->shared_from_this(), boost::asio::placeholders::error));

                        //
                        //	memoize port name
                        //
                        auto port_name = cfg_.get_port_name();
                        CYNG_LOG_TRACE(
                            logger_,
                            "[RDR] session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes => "
                                              << port_name);

                        //
                        //	send to port
                        //
                        registry_.dispatch(
                            port_name,
                            "write",
                            cyng::make_tuple(cyng::buffer_t(buffer_.begin(), buffer_.begin() + bytes_transferred)));

                        //
                        //	continue reading
                        //
                        do_read();
                    } else if (ec != boost::asio::error::connection_aborted) {
                        CYNG_LOG_WARNING(logger_, "[RDR] session stopped: " << ec.message());
                    }
                });
        }

        std::size_t session::get_redirector_id() const { return (channel_) ? channel_->get_id() : 0u; }

        bool session::stop_redirector() {
            if (channel_) {
                channel_->stop();
                return true;
            }
            return false;
        }

        void session::stop_timer() { idle_timer_.cancel(); }

    } // namespace rdr
} // namespace smf
