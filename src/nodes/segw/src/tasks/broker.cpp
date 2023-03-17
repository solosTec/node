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
#include <numeric>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>

namespace smf {

    broker::broker(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, cfg & config, lmn_type type, std::size_t index)
        : state_holder_()   //  initial state
        , sigs_{
            std::bind(&broker::start, this), //    start
            std::bind(&broker::receive, this, std::placeholders::_1), //    receive
            std::bind(&broker::check_status, this, std::placeholders::_1), // check status
            std::bind(&broker::stop, this, std::placeholders::_1)
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , cfg_(config, type)
        , index_(index)
        , endpoints_()
        , socket_(ctl.get_ctx())
        , dispatcher_(ctl.get_ctx())
        , read_buffer_()
        , write_buffer_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"start", "receive", "check-status"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void broker::stop(cyng::eod) { reset(state_holder_, state::value::STOPPED); }

    void broker::reset(state::ptr sp, state::value next) {

        if (sp && !sp->is_stopped()) {
            sp->value_ = next;

            boost::system::error_code ignored_ec;
            switch (next) {
            case state::value::OFFLINE:
                //  required to get a proper error code: connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec); //  connection_aborted
                boost::asio::post(dispatcher_, [this]() { write_buffer_.clear(); });
                break;
            case state::value::STOPPED:
            case state::value::CONNECTED: break;
            default:
                //  required to get a proper error code: connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec); //  connection_aborted
                break;
            }
        }
    }

    void broker::receive(cyng::buffer_t data) {
        if (state_holder_ && state_holder_->is_connected() && !data.empty()) {
            boost::asio::post(dispatcher_, [this, data]() {
                CYNG_LOG_INFO(
                    logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] transmit " << data.size() << " bytes");
                bool const b = write_buffer_.empty();
                write_buffer_.emplace_back(data);
                if (b) {
                    do_write(state_holder_);
                }
            });
        } else {
            CYNG_LOG_WARNING(
                logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] drops " << data.size() << " bytes");
        }
    }

    void broker::start() {
        auto const address = cfg_.get_address(index_);
        auto const port = cfg_.get_port(index_);

        if (port == 0) {
            //
            //  ignore this request
            //
            CYNG_LOG_WARNING(
                logger_,
                "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] connect to " << address << ':' << port
                                    << " - invalid");
            return; //  stop here
        }

        CYNG_LOG_INFO(
            logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] connect to " << address << ':' << port);

        try {
            boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
            //
            //  create new state holder in case configuartion was changed
            //
            state_holder_ = std::make_shared<state::mgr>(r.resolve(address, std::to_string(port)));
            reset(state_holder_, state::value::CONNECTING);
            connect(state_holder_->shared_from_this());

        } catch (std::exception const &ex) {
            CYNG_LOG_WARNING(
                logger_,
                "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] cannot resolve address " << address << ": "
                                    << ex.what());
        }
    }

    void broker::connect(state::ptr sp) {

        BOOST_ASSERT(sp->value_ == state::value::CONNECTING);

        //
        // Start the connect actor.
        //
        start_connect(sp, sp->endpoints_.begin());
    }

    void broker::start_connect(state::ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        //
        //  test if connection was stopped
        //
        if (!sp->is_stopped()) {

            if (endpoint_iter != sp->endpoints_.end()) {
                CYNG_LOG_TRACE(
                    logger_,
                    "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] trying " << endpoint_iter->endpoint() << "...");

                // Start the asynchronous connect operation.
                socket_.async_connect(
                    endpoint_iter->endpoint(), std::bind(&broker::handle_connect, this, sp, std::placeholders::_1, endpoint_iter));
            } else {

                CYNG_LOG_WARNING(logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] connect failed");
                //
                // There are no more endpoints to try. Restart client.
                //
                reset(sp, state::value::OFFLINE);
            }
        }
    }

    void broker::check_status(std::chrono::seconds timeout) {

        //
        //  test if bus was stopped
        //
        if (state_holder_ && !state_holder_->is_stopped()) {

            switch (state_holder_->value_) {
            case state::value::OFFLINE:
                CYNG_LOG_TRACE(logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] status: OFFLINE");
                start();
                break;
            case state::value::CONNECTED:
                CYNG_LOG_TRACE(logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] check status: CONNECTED");
                break;
            case state::value::CONNECTING:
                CYNG_LOG_TRACE(logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] check status: CONNECTING");
                // start();
                break;
            default:
                CYNG_LOG_ERROR(
                    logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] check status: invalid state");
                BOOST_ASSERT_MSG(false, "invalid state");
                break;
            }

            if (auto sp = channel_.lock(); sp) {
                //  repeat
                sp->suspend(
                    timeout,
                    "check-status",
                    std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                    cyng::make_tuple(timeout < std::chrono::seconds(10) ? std::chrono::seconds(10) : timeout));
            }
        }
    }

    void broker::handle_connect(
        state::ptr sp,
        const boost::system::error_code &ec,
        boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            // The async_connect() function automatically opens the socket at the start
            // of the asynchronous operation. If the socket is closed at this time then
            // the timeout handler must have run first.
            if (!socket_.is_open()) {

                CYNG_LOG_WARNING(
                    logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] connect timed out: " << ec.message());

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Check if the connect operation failed before the deadline expired.
            else if (ec) {
                CYNG_LOG_WARNING(
                    logger_,
                    "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] connect error " << ec.value() << ": "
                                        << ec.message());

                // We need to close the socket used in the previous connection attempt
                // before starting a new one.
                boost::system::error_code ec;
                socket_.close(ec);

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Otherwise we have successfully established a connection.
            else {
                CYNG_LOG_INFO(
                    logger_,
                    "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] address resolved "
                                        << endpoint_iter->endpoint());
                reset(sp, state::value::CONNECTED);

                // Start the input actor.
                do_read(sp);

                if (cfg_.has_login()) {

                    boost::asio::post(dispatcher_, [this, sp]() {
                        bool const b = write_buffer_.empty();
                        //
                        //	set login sequence
                        //
                        write_buffer_.emplace_front(cyng::make_buffer("\r\n"));
                        write_buffer_.emplace_front(cyng::make_buffer(cfg_.get_login_sequence(index_)));
                        // Start the write actor.
                        if (b) {
                            do_write(sp);
                        }
                    });
                }
            }
        }
    }

    void broker::do_read(state::ptr sp) {
        //
        //	connect was successful
        //  Start an asynchronous operation to read a newline-delimited message.
        //
        socket_.async_read_some(
            boost::asio::buffer(read_buffer_.data(), read_buffer_.size()),
            std::bind(&broker::handle_read, this, sp, std::placeholders::_1, std::placeholders::_2));
    }

    void broker::handle_read(state::ptr sp, const boost::system::error_code &ec, std::size_t n) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (!ec) {

                //
                //  incoming data will be ignored
                //
                CYNG_LOG_WARNING(
                    logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] received " << n << " bytes");

                do_read(sp);
            } else if (ec != boost::asio::error::connection_aborted) {
                CYNG_LOG_WARNING(
                    logger_,
                    "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] read " << ec.value() << ": " << ec.message());

                //
                //  reset state
                //
                reset(sp, state::value::OFFLINE);
            }
        }
    }

    void broker::do_write(state::ptr sp) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            BOOST_ASSERT(!write_buffer_.empty());

            // Start an asynchronous operation to send a heartbeat message.
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_buffer_.front().data(), write_buffer_.front().size()),
                dispatcher_.wrap(std::bind(&broker::handle_write, this, sp, std::placeholders::_1)));
        }
    }

    void broker::handle_write(state::ptr sp, const boost::system::error_code &ec) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {
            if (!ec) {

                write_buffer_.pop_front();
                if (!write_buffer_.empty()) {
                    do_write(sp);
                }
            } else {
                CYNG_LOG_WARNING(logger_, "[broker-on-start/" << +cfg_.get_index() << "/" << index_ << "] write: " << ec.message());

                reset(sp, state::value::OFFLINE);
            }
        }
    }

    broker_on_demand::broker_on_demand(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        cfg & config, lmn_type type, std::size_t index)
    : state_holder_()
        , sigs_{
            std::bind(&broker_on_demand::receive, this, std::placeholders::_1), //  receive
            std::bind(&broker_on_demand::close_connection, this), // close_connection
            std::bind(&broker_on_demand::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , cfg_(config, type)
        , index_(index)
        , socket_(ctl.get_ctx())
        , dispatcher_(ctl.get_ctx())
        , read_buffer_()
        , write_buffer_() {

        if (auto sp = channel_.lock(); sp) {
            std::size_t slot{0};
            sp->set_channel_name("receive", slot++);
            sp->set_channel_name("close.socket", slot++);
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        // CYNG_LOG_INFO(logger_, "[broker-on-demand] ready: " << target_);
        CYNG_LOG_TRACE(
            logger_,
            "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] write timeout is: " << cfg_.get_timeout(index_).count()
                                 << " seconds");
    }

    void broker_on_demand::start() {
        auto const address = cfg_.get_address(index_);
        auto const port = cfg_.get_port(index_);

        if (port == 0) {
            //
            //  ignore this request
            //
            CYNG_LOG_WARNING(
                logger_,
                "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] connect to " << address << ':' << port
                                     << " - invalid");
            return;
        } else {
            CYNG_LOG_INFO(
                logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] connect to " << address << ':' << port);
        }
        state_holder_.reset();

        try {
            boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
            state_holder_ = std::make_shared<state::mgr>(r.resolve(address, std::to_string(port)));
            reset(state_holder_, state::value::CONNECTING);
            connect(state_holder_->shared_from_this());

        } catch (std::exception const &ex) {
            CYNG_LOG_WARNING(
                logger_,
                "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] cannot resolve address " << address << ": "
                                     << ex.what());
        }
    }

    void broker_on_demand::stop(cyng::eod) { reset(state_holder_, state::value::STOPPED); }

    void broker_on_demand::reset(state::ptr sp, state::value next) {
        if (sp && !sp->is_stopped()) {

            sp->value_ = next;

            boost::system::error_code ignored_ec;
            switch (next) {
            case state::value::OFFLINE:
                //  required to get a proper error code: connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec); //  connection_aborted
                boost::asio::post(dispatcher_, [this]() { write_buffer_.clear(); });
                break;
            case state::value::STOPPED:
                //  stopped
                //  required to get a proper error code: connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec); //  connection_aborted
                break;
            default: break;
            }
        }
    }

    void broker_on_demand::receive(cyng::buffer_t data) {
        if (state_holder_) {
            if (!state_holder_->is_stopped()) {
                switch (state_holder_->value_) {
                case state::value::OFFLINE:
                    CYNG_LOG_TRACE(logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] state: OFFLINE");
                    store(data);
                    start();
                    break;
                case state::value::CONNECTING:
                    CYNG_LOG_TRACE(logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] state: CONNECTING");
                    store(data);
                    break;
                case state::value::CONNECTED:
                    CYNG_LOG_TRACE(logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] state: CONNECTED");
                    send(state_holder_, data);
                    break;
                default: CYNG_LOG_TRACE(logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] state: ?"); break;
                }
            }
        } else {
            store(data);
            start();
        }
    }

    void broker_on_demand::close_connection() {
        if (state_holder_ && !state_holder_->is_stopped()) {
            auto sp = state_holder_->shared_from_this();

            boost::asio::post(dispatcher_, [this, sp]() {
                if (write_buffer_.empty()) {
                    CYNG_LOG_TRACE(
                        logger_,
                        "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] write buffer is empty - close connection");
                    reset(sp, state::value::OFFLINE);
                }
            });
        }
    }

    void broker_on_demand::store(cyng::buffer_t data) {
        if (!data.empty()) {
            boost::asio::post(dispatcher_, [this, data]() {
                //
                //  add to write buffer
                //
                write_buffer_.emplace_back(data);

                //
                //  calculate total buffer size
                //
                auto const total_size = std::accumulate(
                    write_buffer_.begin(),
                    write_buffer_.end(),
                    static_cast<std::size_t>(0),
                    [](std::size_t n, cyng::buffer_t const &buffer) { return n + buffer.size(); });

                CYNG_LOG_INFO(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] stores " << data.size() << " bytes #"
                                         << write_buffer_.size() << " - total: " << total_size);
            });
        }
    }

    void broker_on_demand::send(state::ptr sp, cyng::buffer_t data) {
        if (sp && sp->is_connected() && !data.empty()) {

            boost::asio::post(dispatcher_, [this, sp, data]() {
                bool const b = write_buffer_.empty();
                CYNG_LOG_TRACE(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] write " << data.size()
                                         << " bytes to data cache #" << write_buffer_.size());
                write_buffer_.emplace_back(data);
                if (b) {
                    do_write(sp);
                }
            });
        } else {
            CYNG_LOG_WARNING(
                logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] drops " << data.size() << " bytes");
        }
    }

    void broker_on_demand::do_write(state::ptr sp) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            BOOST_ASSERT(!write_buffer_.empty());
            if (write_buffer_.empty()) {
                CYNG_LOG_WARNING(logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] has empty buffer");
                return;
            }

            // Start an asynchronous operation to send a heartbeat message.
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_buffer_.front().data(), write_buffer_.front().size()),
                dispatcher_.wrap(
                    std::bind(&broker_on_demand::handle_write, this, sp, std::placeholders::_1, std::placeholders::_2)));
        }
    }

    void broker_on_demand::handle_write(state::ptr sp, const boost::system::error_code &ec, std::size_t n) {
        //
        //  test if connection was stopped
        //
        if (!sp->is_stopped()) {

            CYNG_LOG_INFO(
                logger_,
                "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] transmitted " << n << " bytes #"
                                     << write_buffer_.size() << ": " << ec.message());

            if (!ec) {

                if (!write_buffer_.empty())
                    write_buffer_.pop_front();

                if (!write_buffer_.empty()) {
                    do_write(sp);
                } else {
                    //  close socket after 1 second
                    //  is required for the Swistec broker (4A)
                    auto ch_ptr = channel_.lock();
                    if (ch_ptr) {
                        auto const timeout = cfg_.get_timeout(index_);
                        //  handle dispatch errors
                        ch_ptr->suspend(
                            timeout,
                            "close.socket",
                            std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
                    }
                }
            } else {
                CYNG_LOG_WARNING(
                    logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] write: " << ec.message());

                reset(sp, state::value::OFFLINE);
            }
        }
    }

    void broker_on_demand::connect(state::ptr sp) {
        // state_ = state::WAIT;
        BOOST_ASSERT(sp->value_ == state::value::CONNECTING);

        //
        // Start the connect actor.
        //
        start_connect(sp, sp->endpoints_.begin());
    }

    void broker_on_demand::start_connect(state::ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
        //
        //  test if connection was stopped
        //
        if (!sp->is_stopped()) {

            if (endpoint_iter != sp->endpoints_.end()) {
                CYNG_LOG_TRACE(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] trying " << endpoint_iter->endpoint()
                                         << "...");

                // Start the asynchronous connect operation.
                socket_.async_connect(
                    endpoint_iter->endpoint(),
                    std::bind(&broker_on_demand::handle_connect, this, sp, std::placeholders::_1, endpoint_iter));
            } else {

                //
                // There are no more endpoints to try.
                // Reset buffer and go offline
                //
                auto const address = cfg_.get_address(index_);
                auto const port = cfg_.get_port(index_);

                CYNG_LOG_WARNING(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] connect to " << address << ':' << port
                                         << " failed");
                reset(sp, state::value::OFFLINE);
            }
        }
    }

    void broker_on_demand::handle_connect(
        state::ptr sp,
        const boost::system::error_code &ec,
        boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            // The async_connect() function automatically opens the socket at the start
            // of the asynchronous operation. If the socket is closed at this time then
            // the timeout handler must have run first.
            if (!socket_.is_open()) {

                CYNG_LOG_WARNING(
                    logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] connect timed out: " << ec.message());

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Check if the connect operation failed before the deadline expired.
            else if (ec) {
                CYNG_LOG_WARNING(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] connect error " << ec.value() << ": "
                                         << ec.message());

                // We need to close the socket used in the previous connection attempt
                // before starting a new one.
                boost::system::error_code ec;
                socket_.close(ec);

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Otherwise we have successfully established a connection.
            else {
                CYNG_LOG_INFO(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] address resolved "
                                         << endpoint_iter->endpoint());
                reset(sp, state::value::CONNECTED);

                // Start the input actor to detect closed connection
                do_read(sp);

                boost::asio::post(dispatcher_, [this, sp]() {
                    bool const b = write_buffer_.empty();

                    if (cfg_.has_login()) {
                        //
                        //	set login sequence
                        // Start with the login
                        //
                        write_buffer_.emplace_front(cyng::make_buffer("\r\n"));
                        write_buffer_.emplace_front(cyng::make_buffer(cfg_.get_login_sequence(index_)));
                    }

                    //
                    //  start writing
                    //
                    do_write(sp);
                });
            }
        }
    }
    void broker_on_demand::do_read(state::ptr sp) {
        //
        //	connect was successful
        //  Start an asynchronous operation to read a newline-delimited message.
        //
        socket_.async_read_some(
            boost::asio::buffer(read_buffer_.data(), read_buffer_.size()),
            std::bind(&broker_on_demand::handle_read, this, sp, std::placeholders::_1, std::placeholders::_2));
    }

    void broker_on_demand::handle_read(state::ptr sp, const boost::system::error_code &ec, std::size_t n) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (!ec) {

                //
                //  incoming data will be ignored
                //
                CYNG_LOG_WARNING(
                    logger_, "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] received " << n << " bytes");

                //
                //  continue reading
                //
                do_read(sp);

            } else if (ec != boost::asio::error::connection_aborted) {
                CYNG_LOG_WARNING(
                    logger_,
                    "[broker-on-demand/" << +cfg_.get_index() << "/" << index_ << "] read " << ec.value() << ": " << ec.message());
                reset(sp, state::value::OFFLINE);
            }
        }
    }

    namespace state {
        mgr::mgr(boost::asio::ip::tcp::resolver::results_type &&res)
            : value_(value::OFFLINE)
            , endpoints_(std::move(res)) {}
    } // namespace state

} // namespace smf
