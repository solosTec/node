/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/broker.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>

namespace smf {

    broker::broker(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, target const &t, bool login)
        : state_(state::START)
        , sigs_{std::bind(&broker::receive, this, std::placeholders::_1), std::bind(&broker::check_status, this, std::placeholders::_1), std::bind(&broker::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , target_(t)
        , login_(login)
        , endpoints_()
        , socket_(ctl.get_ctx())
        , dispatcher_(ctl.get_ctx())
        , input_buffer_()
        , buffer_write_() {
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("receive", slot++);
            sp->set_channel_name("check-status", slot++);
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        CYNG_LOG_INFO(logger_, "[broker] ready: " << target_);
    }

    void broker::stop(cyng::eod) {

        CYNG_LOG_INFO(logger_, "[broker] stop: " << target_);
        reset(state::STOPPED);
    }

    void broker::reset(state next) {
        state_ = next;
        if (state_ != state::STOPPED) {
            boost::asio::post(dispatcher_, [this]() { buffer_write_.clear(); });
        }
        boost::system::error_code ignored_ec;
        socket_.close(ignored_ec);
    }

    void broker::receive(cyng::buffer_t data) {
        if (is_connected()) {
            CYNG_LOG_INFO(logger_, "[broker] transmit " << data.size() << " bytes to " << target_);

            boost::asio::post(dispatcher_, [this, data]() {
                bool const b = buffer_write_.empty();
                buffer_write_.emplace_back(data);
                if (b)
                    do_write();
            });
        } else {
            CYNG_LOG_WARNING(logger_, "[broker] drops " << data.size() << " bytes to " << target_);
        }
    }

    void broker::start() {
        CYNG_LOG_INFO(logger_, "[broker] start " << target_);

        state_ = state::START;
        boost::asio::post(dispatcher_, [this]() { buffer_write_.clear(); });

        boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
        connect(r.resolve(target_.get_address(), std::to_string(target_.get_port())));
    }

    void broker::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

        state_ = state::WAIT;

        // Start the connect actor.
        endpoints_ = endpoints;
        start_connect(endpoints_.begin());
    }

    void broker::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        if (endpoint_iter != endpoints_.end()) {
            CYNG_LOG_TRACE(logger_, "[broker] trying " << endpoint_iter->endpoint() << "...");

            // Start the asynchronous connect operation.
            socket_.async_connect(
                endpoint_iter->endpoint(), std::bind(&broker::handle_connect, this, std::placeholders::_1, endpoint_iter));
        } else {

            //
            // There are no more endpoints to try. Shut down the client.
            //
            reset(state::START);
        }
    }

    void broker::check_status(std::chrono::seconds timeout) {

        switch (state_) {
        case state::START:
            CYNG_LOG_TRACE(logger_, "[broker] check deadline: start");
            start();
            break;
        case state::CONNECTED:
            // CYNG_LOG_DEBUG(logger_, "[broker] check deadline: connected");
            break;
        case state::WAIT:
            CYNG_LOG_TRACE(logger_, "[broker] check deadline: waiting");
            start();
            break;
        default:
            CYNG_LOG_ERROR(logger_, "[broker] check deadline: invalid state");
            BOOST_ASSERT_MSG(false, "invalid state");
            break;
        }

        auto sp = channel_.lock();
        if (sp)
            sp->suspend(
                timeout, "check-status", cyng::make_tuple(timeout < std::chrono::seconds(10) ? std::chrono::seconds(10) : timeout));
    }

    void broker::handle_connect(
        const boost::system::error_code &ec,
        boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        if (is_stopped())
            return;

        // The async_connect() function automatically opens the socket at the start
        // of the asynchronous operation. If the socket is closed at this time then
        // the timeout handler must have run first.
        if (!socket_.is_open()) {

            CYNG_LOG_WARNING(logger_, "[broker] " << target_ << " connect timed out");

            // Try the next available endpoint.
            start_connect(++endpoint_iter);
        }

        // Check if the connect operation failed before the deadline expired.
        else if (ec) {
            CYNG_LOG_WARNING(logger_, "[broker] " << target_ << " connect error " << ec.value() << ": " << ec.message());

            // We need to close the socket used in the previous connection attempt
            // before starting a new one.
            boost::system::error_code ec;
            socket_.close(ec);

            // Try the next available endpoint.
            start_connect(++endpoint_iter);
        }

        // Otherwise we have successfully established a connection.
        else {
            CYNG_LOG_INFO(logger_, "[broker] " << target_ << " connected to " << endpoint_iter->endpoint());
            state_ = state::CONNECTED;

            // Start the input actor.
            do_read();

            if (login_) {

                boost::asio::post(dispatcher_, [this]() {
                    bool const b = buffer_write_.empty();
                    //
                    //	set login sequence
                    //
                    buffer_write_.emplace_back(cyng::make_buffer(target_.get_login_sequence()));
                    buffer_write_.emplace_back(cyng::make_buffer("\r\n"));
                    // Start the heartbeat actor.
                    if (b)
                        do_write();
                });
            }
        }
    }

    void broker::do_read() {
        //
        //	connect was successful
        //

        // Start an asynchronous operation to read a newline-delimited message.
        boost::asio::async_read_until(
            socket_,
            boost::asio::dynamic_buffer(input_buffer_),
            '\n',
            std::bind(&broker::handle_read, this, std::placeholders::_1, std::placeholders::_2));
    }

    void broker::handle_read(const boost::system::error_code &ec, std::size_t n) {
        if (is_stopped())
            return;

        if (!ec) {
            // Extract the newline-delimited message from the buffer.
            std::string line(input_buffer_.substr(0, n - 1));
            input_buffer_.erase(0, n);

            // Empty messages are heartbeats and so ignored.
            if (!line.empty()) {
                CYNG_LOG_DEBUG(logger_, "[broker] " << target_ << " received " << line);
            }

            do_read();
        } else {
            CYNG_LOG_WARNING(logger_, "[broker] " << target_ << " read " << ec.value() << ": " << ec.message());
            reset(state::START);
        }
    }

    void broker::do_write() {
        if (is_stopped())
            return;

        BOOST_ASSERT(!buffer_write_.empty());

        // Start an asynchronous operation to send a heartbeat message.
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            dispatcher_.wrap(std::bind(&broker::handle_write, this, std::placeholders::_1)));
    }

    void broker::handle_write(const boost::system::error_code &ec) {
        if (is_stopped())
            return;

        if (!ec) {

            buffer_write_.pop_front();
            if (!buffer_write_.empty()) {
                do_write();
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[broker] write " << target_ << ": " << ec.message());

            reset(state::START);
        }
    }
} // namespace smf
