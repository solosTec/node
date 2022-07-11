/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf.h>
#include <smf/ipt/bus.h>
#include <smf/ipt/codes.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_format.h>
#include <smf/ipt/transpiler.h>

#include <cyng/log/record.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#ifdef _DEBUG_IPT
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <smf/sml/unpack.h>
#include <sstream>
#endif

#include <functional>

#include <boost/asio/error.hpp>
#include <boost/bind.hpp>

namespace smf {
    namespace ipt {
        bus::bus(
            boost::asio::io_context &ctx,
            cyng::logger logger,
            toggle::server_vec_t &&tgl,
            std::string model,
            parser::command_cb cb_cmd,
            parser::data_cb cb_stream,
            auth_cb cb_auth)
            : state_holder_()
            , ctx_(ctx)
            , logger_(logger)
            , tgl_(std::move(tgl))
            , model_(model)
            , cb_cmd_(cb_cmd)
            , cb_auth_(cb_auth)
            , socket_(ctx)
            , timer_(ctx)
            , dispatcher_(ctx)
            , serializer_(tgl_.get().sk_)
            , parser_(tgl_.get().sk_, std::bind(&bus::cmd_complete, this, std::placeholders::_1, std::placeholders::_2), cb_stream)
            , buffer_write_()
            , input_buffer_({0})
            , pending_targets_()
            , targets_()
            , opening_channel_()
            , channels_()
            , closing_channels_() {}

        bool bus::is_authorized() const { return (state_holder_) ? state_holder_->is_authorized() : false; }

        void bus::start() {

            CYNG_LOG_INFO(logger_, "[ipt] start client [" << tgl_.get() << "]");
            state_holder_.reset();

            //
            //	connect to IP-T server
            //
            try {
                boost::asio::ip::tcp::resolver r(ctx_);
                //
                //  bus state is START
                //
                state_holder_ = std::make_shared<state>(r.resolve(tgl_.get().host_, tgl_.get().service_));
                connect(state_holder_->shared_from_this());
            } catch (std::exception const &ex) {
                CYNG_LOG_ERROR(logger_, "[ipt] start client: " << ex.what());

                //
                //	switch redundancy
                //
                tgl_.changeover();
                CYNG_LOG_WARNING(logger_, "[ipt] connect failed - switch to " << tgl_.get());

                //
                //  it's safe to to set timer here because this is the START state
                //
                state_holder_ = std::make_shared<state>(boost::asio::ip::tcp::resolver::results_type());
                set_reconnect_timer(std::chrono::seconds(60));
            }
        }

        void bus::connect(state_ptr sp) {

            //
            // Start the connect actor holding a reference
            // to the bus state
            //
            start_connect(sp, sp->endpoints_.begin());
        }

        void bus::stop() {
            // CYNG_LOG_INFO(logger_, "ipt " << tgl_.get() << " stop");
            if (state_holder_) {
                reset(state_holder_, state_value::STOPPED);
            }
        }

        void bus::reset(state_ptr sp, state_value s) {
            if (!sp->is_stopped()) {

                sp->value_ = s;
                boost::system::error_code ignored_ec;
                //  required to get a proper error code: bad_descriptor (EBADF) instead of connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec);
                auto const count = timer_.cancel();
                CYNG_LOG_TRACE(logger_, "[ipt] cancel " << count << " timer");
                if (s != state_value::STOPPED) {

                    buffer_write_.clear();
                    pending_targets_.clear();
                    targets_.clear();
                    opening_channel_.clear();
                    parser_.clear();

                    if (sp->is_authorized()) {

                        //
                        //	signal changed authorization state
                        //
                        auto const ep = boost::asio::ip::tcp::endpoint();
                        cb_auth_(false, ep, ep);
                    }
                }
            }
        }

        void bus::set_reconnect_timer(std::chrono::seconds delay) {

            BOOST_ASSERT_MSG(state_holder_, "state holder is empty");

            auto const count = timer_.expires_after(delay);
            CYNG_LOG_INFO(logger_, "[ipt] set reconnect timer: " << delay.count() << " seconds - " << count << " timer cancelled");
            timer_.async_wait(boost::asio::bind_executor(
                dispatcher_, boost::bind(&bus::reconnect_timeout, this, state_holder_, boost::asio::placeholders::error)));
        }

        void bus::start_connect(state_ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

            //
            //  test if bus was stopped
            //
            if (!sp->is_stopped()) {
                if (endpoint_iter != sp->endpoints_.end()) {

                    CYNG_LOG_INFO(logger_, "[ipt] connect to " << endpoint_iter->endpoint());

                    // Start the asynchronous connect operation.
                    socket_.async_connect(
                        endpoint_iter->endpoint(), std::bind(&bus::handle_connect, this, sp, std::placeholders::_1, endpoint_iter));
                } else {

                    //
                    //  full reset
                    //
                    reset(sp, state_value::START);

                    //
                    //	switch redundancy
                    //
                    tgl_.changeover();
                    CYNG_LOG_WARNING(logger_, "[ipt] connect failed - switch to " << tgl_.get());

                    //
                    //	reconnect after 20 seconds
                    //
                    set_reconnect_timer(boost::asio::chrono::seconds(20));
                }
            }
        }

        void bus::handle_connect(
            state_ptr sp,
            const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

            //
            //  test if bus was stopped
            //
            if (!sp->is_stopped()) {

                BOOST_ASSERT_MSG(sp->has_state(state_value::START), "START state expected");

                // The async_connect() function automatically opens the socket at the
                // start of the asynchronous operation. If the socket is closed at this
                // time then the timeout handler must have run first.
                if (!socket_.is_open()) {

                    CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] connect timed out");

                    // Try the next available endpoint.
                    start_connect(sp, ++endpoint_iter);
                }

                // Check if the connect operation failed before the deadline expired.
                else if (ec) {
                    CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] connect error: " << ec.message());

                    // We need to close the socket used in the previous connection attempt
                    // before starting a new one.
                    socket_.close();

                    // Try the next available endpoint.
                    start_connect(sp, ++endpoint_iter);
                }

                // Otherwise we have successfully established a connection.
                else {
                    CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] connected to " << endpoint_iter->endpoint());
                    sp->value_ = state_value::CONNECTED;

                    //
                    //	send login sequence
                    //
                    auto const srv = tgl_.get();
                    if (srv.scrambled_) {

                        //
                        //	use a random key
                        //
                        auto const sk = gen_random_sk();

                        CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] sk = " << ipt::to_string(sk));
                        parser_.set_sk(sk);
                        send(sp, std::bind(&serializer::req_login_scrambled, &serializer_, srv.account_, srv.pwd_, sk));
                    } else {
                        send(sp, std::bind(&serializer::req_login_public, &serializer_, srv.account_, srv.pwd_));
                    }

                    //  Start the input actor.
                    //  Sorward state.
                    do_read(sp);
                }
            }
        }

        void bus::reconnect_timeout(state_ptr sp, boost::system::error_code const &ec) {
            if (sp && !sp->is_stopped()) {

                if (!ec) {
                    CYNG_LOG_INFO(logger_, "[ipt] reconnect to " << tgl_.get().host_);
                    if (!sp->is_authorized()) {
                        start();
                    }
                } else if (ec == boost::asio::error::operation_aborted) {
                    // CYNG_LOG_TRACE(logger_, "[ipt] reconnect timer cancelled");
                } else {
                    CYNG_LOG_WARNING(logger_, "[ipt] reconnect timer: " << ec.message());
                }
            }
        }

        void bus::do_read(state_ptr sp) {

            // Start an asynchronous operation to read a newline-delimited message.
            socket_.async_read_some(
                boost::asio::buffer(input_buffer_),
                std::bind(&bus::handle_read, this, sp, std::placeholders::_1, std::placeholders::_2));
        }

        void bus::do_write(state_ptr sp) {
            //
            //  test if bus was stopped
            //
            if (!sp->is_stopped()) {

                // BOOST_ASSERT(!buffer_write_.empty());
#ifdef _DEBUG
                CYNG_LOG_TRACE(
                    logger_,
                    "ipt [" << tgl_.get() << "] send #" << buffer_write_.size() << ": " << buffer_write_.front().size()
                            << " bytes to " << socket_.remote_endpoint());
#else
                CYNG_LOG_TRACE(
                    logger_,
                    "ipt [" << tgl_.get() << "] send #" << buffer_write_.size() << ": " << buffer_write_.front().size()
                            << " bytes");
#endif

#ifdef __DEBUG_IPT
                {
                    //  scrambled data
                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, buffer_write_.front().begin(), buffer_write_.front().end());
                    auto const dmp = ss.str();
                    CYNG_LOG_DEBUG(logger_, "[ipt] write #" << buffer_write_.size() << ":\n" << dmp);
                }
#endif

                // Start an asynchronous operation to send a heartbeat message.
                boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
                    dispatcher_.wrap(std::bind(&bus::handle_write, this, sp, std::placeholders::_1)));
            }
        }

        void bus::handle_read(state_ptr sp, const boost::system::error_code &ec, std::size_t n) {
            //
            //  test if bus was stopped
            //
            if (!sp->is_stopped()) {

                if (!ec) {
                    CYNG_LOG_DEBUG(logger_, "ipt [" << tgl_.get() << "] received " << n << " bytes " << socket_.remote_endpoint());

                    //
                    //  get de-obfuscated data
                    //
                    auto const data = parser_.read(input_buffer_.begin(), input_buffer_.begin() + n);
#ifdef __DEBUG_IPT
                    {
                        std::stringstream ss;
                        cyng::io::hex_dump<8> hd;
                        hd(ss, data.begin(), data.end());
                        auto const dmp = ss.str();
                        CYNG_LOG_DEBUG(
                            logger_,
                            "received " << n << " bytes ipt data from [" << socket_.remote_endpoint() << "]:\n"
                                        << dmp);
                    }
#else
                    boost::ignore_unused(data);
#endif

                    //
                    //	continue reading
                    //
                    do_read(sp);

                } else {
                    CYNG_LOG_ERROR(logger_, "ipt [" << tgl_.get() << "] on receive " << ec.value() << ": " << ec.message());

                    //
                    //  cleanup
                    //
                    reset(sp, state_value::START);

                    //
                    //	reconnect after 10/20 seconds
                    //
                    switch (ec.value()) {
                    case boost::asio::error::bad_descriptor:
                        //	closed by itself
                        set_reconnect_timer(boost::asio::chrono::seconds(100));
                        break;
                    case boost::asio::error::connection_reset:
                        //  closes from peer
                        set_reconnect_timer(boost::asio::chrono::seconds(10));
                        break;
                    default: set_reconnect_timer(boost::asio::chrono::seconds(20)); break;
                    }
                }
            }
        }

        void bus::handle_write(state_ptr sp, const boost::system::error_code &ec) {
            //
            //  test if bus was stopped
            //
            if (!sp->is_stopped()) {

                if (!ec) {

                    CYNG_LOG_TRACE(logger_, "ipt [" << tgl_.get() << "] write buffer #" << buffer_write_.size());
                    if (buffer_write_.empty()) {
                        CYNG_LOG_FATAL(logger_, "ipt write empty buffer");
                    }
                    // BOOST_ASSERT(!buffer_write_.empty());
                    buffer_write_.pop_front();
                    if (!buffer_write_.empty()) {
                        do_write(sp);
                    }
                } else {
                    CYNG_LOG_ERROR(logger_, "ipt write [" << tgl_.get() << "]: " << ec.message());

                    reset(sp, state_value::START);
                }
            }
        }

        void bus::send(state_ptr sp, std::function<cyng::buffer_t()> f) {
            boost::asio::post(dispatcher_, [this, sp, f]() {
                if (sp) {
                    //  if true - already writing
                    auto const b = buffer_write_.empty();
                    buffer_write_.push_back(f());
                    //
                    //  it's essential not to start a new write operation
                    //  if one is already running.
                    //
                    if (b) {
                        do_write(sp);
                    }
                }
            });
        }

        void bus::cmd_complete(header const &h, cyng::buffer_t &&body) {

            BOOST_ASSERT(state_holder_);
            BOOST_ASSERT(!state_holder_->is_stopped());

            CYNG_LOG_TRACE(logger_, "ipt [" << tgl_.get() << "] cmd " << command_name(h.command_));

            switch (to_code(h.command_)) {
            case code::TP_RES_OPEN_PUSH_CHANNEL: res_open_push_channel(h, std::move(body)); break;
            case code::TP_RES_CLOSE_PUSH_CHANNEL:
                res_close_push_channel(h, std::move(body));
                // cb_cmd_(h, std::move(body));
                break;
            case code::TP_REQ_PUSHDATA_TRANSFER: pushdata_transfer(h, std::move(body)); break;
            case code::TP_RES_PUSHDATA_TRANSFER:
                //
                // ToDo: dispatch to channel owner
                //
                // std::tuple<std::uint32_t, std::uint32_t, std::uint8_t, std::uint8_t, cyng::buffer_t>
                // tp_req_pushdata_transfer(cyng::buffer_t &&data);
                cb_cmd_(h, std::move(body));
                break;
            case code::TP_REQ_OPEN_CONNECTION: open_connection(tp_req_open_connection(std::move(body)), h.sequence_); break;
            case code::TP_RES_OPEN_CONNECTION: cb_cmd_(h, std::move(body)); break;
            case code::TP_REQ_CLOSE_CONNECTION: close_connection(h.sequence_); break;
            case code::TP_RES_CLOSE_CONNECTION: cb_cmd_(h, std::move(body)); break;
            case code::APP_REQ_PROTOCOL_VERSION:
                send(state_holder_, std::bind(&serializer::res_protocol_version, &serializer_, h.sequence_, 1));
                break;
            case code::APP_REQ_SOFTWARE_VERSION:
                send(state_holder_, std::bind(&serializer::res_software_version, &serializer_, h.sequence_, SMF_VERSION_NAME));
                break;
            case code::APP_REQ_DEVICE_IDENTIFIER:
                send(state_holder_, std::bind(&serializer::res_device_id, &serializer_, h.sequence_, model_));
                break;
            case code::APP_REQ_DEVICE_AUTHENTIFICATION:
                send(
                    state_holder_,
                    std::bind(
                        &serializer::res_device_auth,
                        &serializer_,
                        h.sequence_,
                        tgl_.get().account_,
                        tgl_.get().pwd_,
                        tgl_.get().account_ //	number
                        ,
                        model_));
                break;
            case code::APP_REQ_DEVICE_TIME:
                send(state_holder_, std::bind(&serializer::res_device_time, &serializer_, h.sequence_));
                break;
            case code::CTRL_RES_LOGIN_PUBLIC: res_login(std::move(body)); break;
            case code::CTRL_RES_LOGIN_SCRAMBLED:
                res_login(std::move(body));
                break;

                // case code::CTRL_REQ_LOGOUT:
                //	break;
                // case code::CTRL_RES_LOGOUT:
                //	break;

            case code::CTRL_REQ_REGISTER_TARGET:
                send(state_holder_, std::bind(&serializer::res_unknown_command, &serializer_, h.sequence_, h.command_));
                break;
            case code::CTRL_RES_REGISTER_TARGET: res_register_target(h, std::move(body)); break;
            case code::CTRL_REQ_DEREGISTER_TARGET:
                send(state_holder_, std::bind(&serializer::res_unknown_command, &serializer_, h.sequence_, h.command_));
                break;
            case code::CTRL_RES_DEREGISTER_TARGET: cb_cmd_(h, std::move(body)); break;
            case code::CTRL_REQ_WATCHDOG: req_watchdog(h, std::move(body)); break;

            case code::UNKNOWN:
            default: send(state_holder_, std::bind(&serializer::res_unknown_command, &serializer_, h.sequence_, h.command_)); break;
            }
        }

        void bus::res_login(cyng::buffer_t &&data) {
            if (state_holder_) {
                auto const [res, watchdog, redirect] = ctrl_res_login(std::move(data));
                auto r = make_login_response(res);
                if (r.is_success()) {

                    CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] " << r.get_response_name());

                    //
                    //	update state
                    //
                    state_holder_->value_ = state_value::AUTHORIZED;

                    //
                    //	set watchdog
                    //
                    if (watchdog != 0) {
                        CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] set watchdog: " << watchdog << " minutes");

                        //
                        //	ToDo: start watchdog
                        //
                    }

                    //
                    //	signal changed authorization state
                    //
                    cb_auth_(true, socket_.local_endpoint(), socket_.remote_endpoint());
                } else {
                    CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] login failed: " << r.get_response_name());
                    boost::system::error_code ignored_ec;
                    //
                    //  This triggers a reconnect.
                    //  required to get a proper error code: bad_descriptor (EBADF) instead of connection_aborted
                    //
                    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                    socket_.close(ignored_ec);
                }
            }
        }

        bool bus::register_target(std::string name, cyng::channel_weak wp) {

            if (!name.empty() && wp.lock()) {

                //
                //	send request
                //
                send(state_holder_, [=, this]() -> cyng::buffer_t {
                    auto r = serializer_.req_register_push_target(
                        name,
                        std::numeric_limits<std::uint16_t>::max(), //	0xffff
                        1);

                    //
                    //	send register command to ip-t server
                    //
                    pending_targets_.emplace(r.second, std::make_pair(name, wp));

                    //  buffer
                    return r.first;
                });

                return true;
            } else {
                CYNG_LOG_WARNING(logger_, "[ipt] cannot register target: name or channel is empty");
            }
            return false;
        }

        bool bus::open_channel(push_channel pc_cfg, cyng::channel_weak wp) {

            if (!wp.expired() && !pc_cfg.target_.empty()) {

                //
                //	send request
                //
                send(state_holder_, [=, this]() -> cyng::buffer_t {
                    auto const r = serializer_.req_open_push_channel(
                        pc_cfg.target_, pc_cfg.account_, pc_cfg.number_, pc_cfg.version_, pc_cfg.id_, pc_cfg.timeout_);

                    //
                    //  update list of pending channel openings
                    //
                    opening_channel_.emplace(r.second, std::make_pair(pc_cfg, wp));

                    CYNG_LOG_INFO(
                        logger_,
                        "[ipt] open channel \"" << pc_cfg.target_ << "\" - #" << opening_channel_.size() << " pending request(s)");

                    //  buffer
                    return r.first;
                });

                return true;
            } else {
                CYNG_LOG_WARNING(logger_, "[ipt] cannot open push chanel: name or channel is empty");
            }
            return false;
        }

        bool bus::close_channel(std::uint32_t channel, cyng::channel_weak wp) {
            BOOST_ASSERT(!wp.expired());

            if (!wp.expired()) {
                send(state_holder_, [=, this]() -> cyng::buffer_t {
                    CYNG_LOG_INFO(logger_, "[ipt] close channel: " << channel);

                    BOOST_ASSERT_MSG(
                        state_holder_->has_state(state_value::AUTHORIZED) || state_holder_->has_state(state_value::LINKED),
                        "AUTHORIZED state expected");
                    auto const r = serializer_.req_close_push_channel(channel);

                    //
                    //  update list of pending channel closings
                    //
                    closing_channels_.emplace(r.second, std::make_pair(channel, wp));

                    //
                    //	send "close push channel" command to ip-t server
                    //
                    return r.first;
                });
                return true;
            }

            return false;
        }

        void bus::res_open_push_channel(header const &h, cyng::buffer_t &&body) {
            //
            //	read message body (TP_RES_OPEN_CONNECTION)
            //
            // response code, channel, source, packet size, window size, status, target count
            auto const [res, channel, source, packet_size, window_size, status, count] = tp_res_open_push_channel(std::move(body));

            boost::asio::post(dispatcher_, [this, h, res, channel, source, packet_size, window_size, status, count]() {
                auto const pos = opening_channel_.find(h.sequence_);
                if (pos != opening_channel_.end()) {

                    auto sp = pos->second.second.lock();
                    if (sp) {
                        if (tp_res_open_push_channel_policy::is_success(res)) {
                            CYNG_LOG_INFO(
                                logger_,
                                "[ipt] cmd " << ipt::command_name(h.command_) << ": "
                                             << tp_res_open_push_channel_policy::get_response_name(res) << ", " << channel << ":"
                                             << source << ":" << pos->second.first.target_);

                        } else {
                            CYNG_LOG_WARNING(
                                logger_,
                                "[ipt] cmd " << ipt::command_name(h.command_) << ": "
                                             << tp_res_open_push_channel_policy::get_response_name(res) << ", " << channel << ":"
                                             << source << ":" << pos->second.first.target_);
                        }

                        //
                        //  update channels list
                        //
                        channels_.emplace(channel, std::move(pos->second));

                        sp->dispatch(
                            "on.channel.open",
                            tp_res_open_push_channel_policy::is_success(res),
                            channel,
                            source,
                            count,
                            pos->second.first.target_);
                    }

                    //
                    //  cleanup list
                    //
                    opening_channel_.erase(pos);

                    CYNG_LOG_INFO(logger_, "[ipt] open channel #" << opening_channel_.size() << " pending request(s)");

                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << ": missing list entry");
                }
            });
        }

        void bus::res_close_push_channel(header const &h, cyng::buffer_t &&body) {
            //
            //	read message body (TP_RES_CLOSE_PUSH_CHANNEL)
            //
            // std::tuple<std::uint8_t, std::uint32_t>
            auto const [res, channel] = tp_res_close_push_channel(std::move(body));
            boost::asio::post(dispatcher_, [this, h, res, channel]() {
                auto const pos = closing_channels_.find(h.sequence_);
                if (pos != closing_channels_.end()) {

                    auto sp = pos->second.second.lock();
                    if (sp) {

                        if (tp_res_close_push_channel_policy::is_success(res)) {
                            CYNG_LOG_INFO(
                                logger_,
                                "[ipt] cmd " << ipt::command_name(h.command_) << ": "
                                             << tp_res_close_push_channel_policy::get_response_name(res) << ", " << channel << "/"
                                             << pos->second.first);

                        } else {
                            CYNG_LOG_WARNING(
                                logger_,
                                "[ipt] cmd " << ipt::command_name(h.command_) << ": "
                                             << tp_res_close_push_channel_policy::get_response_name(res) << ", " << channel << "/"
                                             << pos->second.first);
                        }

                        //
                        // dispatch
                        //
                        sp->dispatch("on.channel.close", tp_res_close_push_channel_policy::is_success(res), channel);
                    }

                    //
                    //  cleanup list
                    //
                    closing_channels_.erase(pos);

                    //
                    //  remove from channels list
                    //
                    if (channels_.erase(channel) == 0) {
                        CYNG_LOG_WARNING(logger_, "[ipt] cmd - channel " << channel << " not found in open channel list");
                    }

                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << ": missing list entry");
                }
            });
        }

        void bus::res_register_target(header const &h, cyng::buffer_t &&body) {
            //
            //	read message body
            //
            auto const [res, channel] = ctrl_res_register_target(std::move(body));

            //
            //	sync with strand
            //
            boost::asio::post(dispatcher_, [this, h, res, channel]() {
                auto const pos = pending_targets_.find(h.sequence_);
                if (pos != pending_targets_.end()) {

                    CYNG_LOG_INFO(
                        logger_,
                        "[ipt] cmd " << ipt::command_name(h.command_) << ": "
                                     << ctrl_res_register_target_policy::get_response_name(res) << ", " << channel << ":"
                                     << pos->second.first);

                    BOOST_ASSERT_MSG(!pos->second.first.empty(), "no target name");

                    //	name/channel
                    auto sp = pos->second.second.lock();
                    if (sp) {
                        if (ctrl_res_register_target_policy::is_success(res)) {
                            targets_.emplace(channel, std::move(pos->second));
                        } else {
                            CYNG_LOG_ERROR(logger_, "[ipt] register target " << pos->second.first << " failed: stop session");
                            //
                            //	stop task
                            //
                            sp->stop();
                        }
                    }

                    //
                    //	cleanup list
                    //
                    pending_targets_.erase(pos);
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << ": missing registrant");
                }
            });

#ifdef __DEBUG_IPT
            //
            //  list all registered targets
            //
            for (auto const &e : targets_) {
                CYNG_LOG_DEBUG(logger_, "[ipt] target " << e.first << ": " << e.second.first);
            }
#endif
        }

        void bus::pushdata_transfer(header const &h, cyng::buffer_t &&body) {
            /**
             * @return channel, source, status, block and data
             */
            auto [channel, source, status, block, data] = tp_req_pushdata_transfer(std::move(body));
            CYNG_LOG_TRACE(
                logger_,
                "[ipt] cmd " << ipt::command_name(h.command_) << " " << channel << ':' << source << " - " << data.size()
                             << " bytes");

            //
            //	sync with strand
            //
            boost::asio::post(dispatcher_, [this, h, channel, source, status, block, data]() {
                BOOST_ASSERT_MSG(!data.empty(), "no push data");

                auto const pos = targets_.find(channel);
                if (pos != targets_.end()) {

                    //	name/channel
                    auto sp = pos->second.second.lock();
                    if (sp) {
                        if (pos->second.first.empty()) {
                            CYNG_LOG_WARNING(logger_, "[ipt] target " << channel << ':' << source << " without name");
#ifdef _DEBUG_IPT
                            //
                            //  list all registered targets
                            //
                            // for (auto const &e : targets_) {
                            //    CYNG_LOG_DEBUG(logger_, "[ipt] target " << e.first << ": " << e.second.first);
                            //}
#endif
                        }
                        // BOOST_ASSERT_MSG(!pos->second.first.empty(), "no target name");
                        std::string const target = pos->second.first;
                        sp->dispatch("pushdata.transfer", channel, source, data, target);
                    } else {
                        CYNG_LOG_WARNING(logger_, "[ipt] remove target " << channel << ':' << source);
                        //
                        //	task removed - clean up
                        //
                        targets_.erase(pos);
                    }
                } else {
                    CYNG_LOG_WARNING(
                        logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << channel << ':' << source << " dropped");
                }
            });

#ifdef _DEBUG_IPT
            //
            //  list all registered targets
            //
            // for (auto const &e : targets_) {
            //    CYNG_LOG_DEBUG(logger_, "[ipt] target " << e.first << ": " << e.second.first);
            //}
#endif
        }

        void bus::req_watchdog(header const &h, cyng::buffer_t &&body) {
            BOOST_ASSERT(body.empty());
            CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << +h.sequence_);
            send(state_holder_, std::bind(&serializer::res_watchdog, &serializer_, h.sequence_));
        }

        void bus::open_connection(std::string number, sequence_t seq) {
            // BOOST_ASSERT_MSG(sp->has_state(state_value::AUTHORIZED), "AUTHORIZED state expected");
            if (state_holder_ && state_holder_->has_state(state_value::AUTHORIZED)) {
                state_holder_->value_ = state_value::LINKED;
                CYNG_LOG_TRACE(logger_, "[ipt] linked to " << number);
                send(
                    state_holder_,
                    std::bind(&serializer::res_open_connection, &serializer_, seq, tp_res_open_connection_policy::DIALUP_SUCCESS));
            } else {
                send(
                    state_holder_,
                    std::bind(&serializer::res_open_connection, &serializer_, seq, tp_res_open_connection_policy::BUSY));
            }
        }

        void bus::close_connection(sequence_t seq) {
            // BOOST_ASSERT_MSG(sp->has_state(state_value::LINKED), "LINKED state expected");
            if (state_holder_ && state_holder_->has_state(state_value::LINKED)) {
                state_holder_->value_ = state_value::AUTHORIZED;
                send(
                    state_holder_,
                    std::bind(
                        &serializer::res_close_connection,
                        &serializer_,
                        seq,
                        tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED));
                CYNG_LOG_TRACE(logger_, "[ipt] unlinked");
            } else {
                send(
                    state_holder_,
                    std::bind(
                        &serializer::res_close_connection,
                        &serializer_,
                        seq,
                        tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED));
            }
        }

        void bus::transmit(std::pair<std::uint32_t, std::uint32_t> id, cyng::buffer_t data) {
            //  status = 0xc1
            CYNG_LOG_TRACE(logger_, "[ipt] push " << data.size() << " bytes over channel " << id.first << ':' << id.second);
#ifdef _DEBUG_IPT
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, data.begin(), data.end());
                auto const dmp = ss.str();
                CYNG_LOG_DEBUG(
                    logger_,
                    "[ipt] write " << data.size() << " bytes over channel " << id.first << ':' << id.second << ":\n"
                                   << dmp);
            }
#endif

            send(state_holder_, std::bind(&serializer::req_transfer_push_data, &serializer_, id.first, id.second, 0xc1, 0, data));
        }

        void bus::transfer(cyng::buffer_t &&data) {
            CYNG_LOG_TRACE(logger_, "[ipt] transfer " << data.size() << " bytes");
            //  escape and scramble data
            send(state_holder_, [=, this]() mutable -> cyng::buffer_t { return serializer_.escape_data(std::move(data)); });
        }

        bus::state::state(boost::asio::ip::tcp::resolver::results_type &&res)
            : value_(state_value::START)
            , endpoints_(std::move(res)) {}

        bool is_null(channel_id const &id) { return id == std::make_pair(0u, 0u); }
        void init(channel_id &id) { id = std::make_pair(0u, 0u); }

    } // namespace ipt
} // namespace smf
