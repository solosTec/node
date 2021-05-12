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
#include <smf/ipt/transpiler.h>

#include <cyng/log/record.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <functional>

#include <boost/bind.hpp>

namespace smf {
    namespace ipt {
        bus::bus(
            boost::asio::io_context &ctx, cyng::logger logger, toggle::server_vec_t &&tgl, std::string model,
            parser::command_cb cb_cmd, parser::data_cb cb_stream, auth_cb cb_auth)
            : state_(state::START),
              ctx_(ctx),
              logger_(logger),
              tgl_(std::move(tgl)),
              model_(model),
              cb_cmd_(cb_cmd),
              cb_auth_(cb_auth),
              endpoints_(),
              socket_(ctx),
              timer_(ctx),
              dispatcher_(ctx),
              serializer_(tgl_.get().sk_),
              parser_(tgl_.get().sk_, std::bind(&bus::cmd_complete, this, std::placeholders::_1, std::placeholders::_2), cb_stream),
              buffer_write_(),
              input_buffer_(),
              registrant_(),
              targets_() {}

        bus::~bus() {}

        void bus::start() {
            BOOST_ASSERT(state_ == state::START);

            CYNG_LOG_INFO(logger_, "[ipt] start client [" << tgl_.get() << "]");

            //
            //	connect to IP-T server
            //
            try {
                boost::asio::ip::tcp::resolver r(ctx_);
                connect(r.resolve(tgl_.get().host_, tgl_.get().service_));
            } catch (std::exception const &ex) {
                CYNG_LOG_ERROR(logger_, "[ipt] start client: " << ex.what());

                //
                //	switch redundancy
                //
                tgl_.changeover();
                CYNG_LOG_WARNING(logger_, "[ipt] connect failed - switch to " << tgl_.get());

                set_reconnect_timer(std::chrono::seconds(60));
            }
        }

        void bus::stop() {
            CYNG_LOG_INFO(logger_, "ipt " << tgl_.get() << " stop");
            reset(state::STOPPED);
        }

        void bus::reset(state s) {
            if (!is_stopped()) {

                if (s == state::STOPPED)
                    state_ = s;
                buffer_write_.clear();
                boost::system::error_code ignored_ec;
                socket_.close(ignored_ec);
                timer_.cancel();

                if (is_authorized()) {

                    //
                    //	signal changed authorization state
                    //
                    cb_auth_(false);
                }
                state_ = s;
            }
        }

        void bus::set_reconnect_timer(std::chrono::seconds delay) {

            if (!is_stopped()) {
                CYNG_LOG_TRACE(logger_, "[ipt] set reconnect timer: " << delay.count() << " seconds");
                timer_.expires_after(delay);
                timer_.async_wait(boost::asio::bind_executor(
                    dispatcher_, boost::bind(&bus::reconnect_timeout, this, boost::asio::placeholders::error)));
            }
        }

        void bus::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

            // Start the connect actor.
            endpoints_ = endpoints;
            start_connect(endpoints_.begin());
        }

        void bus::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
            if (endpoint_iter != endpoints_.end()) {

                CYNG_LOG_INFO(logger_, "[ipt] connect to " << endpoint_iter->endpoint());

                // Start the asynchronous connect operation.
                socket_.async_connect(
                    endpoint_iter->endpoint(), std::bind(&bus::handle_connect, this, std::placeholders::_1, endpoint_iter));
            } else {

                // There are no more endpoints to try. Shut down the client.
                reset(state::START);

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

        void bus::handle_connect(
            const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

            if (is_stopped())
                return;

            // The async_connect() function automatically opens the socket at the
            // start of the asynchronous operation. If the socket is closed at this
            // time then the timeout handler must have run first.
            if (!socket_.is_open()) {

                CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] connect timed out");

                // Try the next available endpoint.
                start_connect(++endpoint_iter);
            }

            // Check if the connect operation failed before the deadline expired.
            else if (ec) {
                CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] connect error: " << ec.message());

                // We need to close the socket used in the previous connection attempt
                // before starting a new one.
                socket_.close();

                // Try the next available endpoint.
                start_connect(++endpoint_iter);
            }

            // Otherwise we have successfully established a connection.
            else {
                CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] connected to " << endpoint_iter->endpoint());
                state_ = state::CONNECTED;

                //
                //	send login sequence
                //
                auto const srv = tgl_.get();
                if (srv.scrambled_) {

                    //
                    //	use a random key
                    //
                    auto const sk = gen_random_sk();

                    send(serializer_.req_login_scrambled(srv.account_, srv.pwd_, sk));
                    parser_.set_sk(sk);
                } else {
                    send(serializer_.req_login_public(srv.account_, srv.pwd_));
                }

                // Start the input actor.
                do_read();
            }
        }

        void bus::reconnect_timeout(boost::system::error_code const &ec) {
            if (is_stopped())
                return;

            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[ipt] reconnect timeout " << ec);
                if (!is_authorized()) {
                    start();
                }
            } else if (ec == boost::asio::error::operation_aborted) {
                // CYNG_LOG_TRACE(logger_, "[ipt] reconnect timer cancelled");
            } else {
                CYNG_LOG_WARNING(logger_, "[ipt] reconnect timer: " << ec.message());
            }
        }

        void bus::do_read() {
            // Start an asynchronous operation to read a newline-delimited message.
            socket_.async_read_some(
                boost::asio::buffer(input_buffer_),
                std::bind(&bus::handle_read, this, std::placeholders::_1, std::placeholders::_2));
        }

        void bus::do_write() {
            if (is_stopped())
                return;

            CYNG_LOG_TRACE(logger_, "ipt [" << tgl_.get() << "] write: " << buffer_write_.front().size() << " bytes");

            // Start an asynchronous operation to send a heartbeat message.
            boost::asio::async_write(
                socket_, boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
                dispatcher_.wrap(std::bind(&bus::handle_write, this, std::placeholders::_1)));
        }

        void bus::handle_read(const boost::system::error_code &ec, std::size_t n) {
            if (is_stopped())
                return;

            if (!ec) {
                CYNG_LOG_DEBUG(logger_, "ipt [" << tgl_.get() << "] received " << n << " bytes");

                parser_.read(input_buffer_.begin(), input_buffer_.begin() + n);

                //
                //	continue reading
                //
                do_read();
            } else {
                CYNG_LOG_ERROR(logger_, "ipt [" << tgl_.get() << "] on receive " << ec.value() << ": s" << ec.message());

                reset(state::START);

                //
                //	reconnect after 10/20 seconds
                //
                switch (ec.value()) {
                case boost::asio::error::bad_descriptor:
                    //	closed by itself
                    set_reconnect_timer(boost::asio::chrono::seconds(100));
                    break;
                case boost::asio::error::connection_reset:
                    set_reconnect_timer(boost::asio::chrono::seconds(10));
                    break;
                default:
                    set_reconnect_timer(boost::asio::chrono::seconds(20));
                    break;
                }
            }
        }

        void bus::handle_write(const boost::system::error_code &ec) {
            if (is_stopped())
                return;

            if (!ec) {

                buffer_write_.pop_front();
                if (!buffer_write_.empty()) {
                    do_write();
                }
            } else {
                CYNG_LOG_ERROR(logger_, "ipt [" << tgl_.get() << "] on heartbeat: " << ec.message());

                reset(state::START);
            }
        }

        void bus::send(cyng::buffer_t data) {
            boost::asio::post(dispatcher_, [this, data]() {
                bool const b = buffer_write_.empty();
                buffer_write_.push_back(data);
                if (b)
                    do_write();
            });
        }

        void bus::cmd_complete(header const &h, cyng::buffer_t &&body) {
            CYNG_LOG_TRACE(logger_, "ipt [" << tgl_.get() << "] cmd " << command_name(h.command_));

            // auto instructions = gen_instructions(h, std::move(body));

            switch (to_code(h.command_)) {
            case code::TP_RES_OPEN_PUSH_CHANNEL:
                cb_cmd_(h, std::move(body));
                break;
            case code::TP_RES_CLOSE_PUSH_CHANNEL:
                cb_cmd_(h, std::move(body));
                break;
            case code::TP_REQ_PUSHDATA_TRANSFER:
                pushdata_transfer(h, std::move(body));
                break;
            case code::TP_RES_PUSHDATA_TRANSFER:
                cb_cmd_(h, std::move(body));
                break;
            case code::TP_REQ_OPEN_CONNECTION:
                open_connection(tp_req_open_connection(std::move(body)), h.sequence_);
                break;
            case code::TP_RES_OPEN_CONNECTION:
                cb_cmd_(h, std::move(body));
                break;
            case code::TP_REQ_CLOSE_CONNECTION:
                close_connection(h.sequence_);
                break;
            case code::TP_RES_CLOSE_CONNECTION:
                cb_cmd_(h, std::move(body));
                break;
            case code::APP_REQ_PROTOCOL_VERSION:
                send(serializer_.res_protocol_version(h.sequence_, 1));
                break;
            case code::APP_REQ_SOFTWARE_VERSION:
                send(serializer_.res_software_version(h.sequence_, SMF_VERSION_NAME));
                break;
            case code::APP_REQ_DEVICE_IDENTIFIER:
                send(serializer_.res_device_id(h.sequence_, model_));
                break;
            case code::APP_REQ_DEVICE_AUTHENTIFICATION:
                send(serializer_.res_device_auth(
                    h.sequence_, tgl_.get().account_, tgl_.get().pwd_,
                    tgl_.get().account_ //	number
                    ,
                    model_));
                break;
            case code::APP_REQ_DEVICE_TIME:
                send(serializer_.res_device_time(h.sequence_));
                break;
            case code::CTRL_RES_LOGIN_PUBLIC:
                res_login(std::move(body));
                break;
            case code::CTRL_RES_LOGIN_SCRAMBLED:
                res_login(std::move(body));
                break;

                // case code::CTRL_REQ_LOGOUT:
                //	break;
                // case code::CTRL_RES_LOGOUT:
                //	break;

            case code::CTRL_REQ_REGISTER_TARGET:
                send(serializer_.res_unknown_command(h.sequence_, h.command_));
                break;
            case code::CTRL_RES_REGISTER_TARGET:
                res_register_target(h, std::move(body));
                break;
            case code::CTRL_REQ_DEREGISTER_TARGET:
                send(serializer_.res_unknown_command(h.sequence_, h.command_));
                break;
            case code::CTRL_RES_DEREGISTER_TARGET:
                cb_cmd_(h, std::move(body));
                break;
            case code::CTRL_REQ_WATCHDOG:
                req_watchdog(h, std::move(body));
                break;

            case code::UNKNOWN:
            default:
                send(serializer_.res_unknown_command(h.sequence_, h.command_));
                break;
            }
        }

        void bus::res_login(cyng::buffer_t &&data) {
            auto const [res, watchdog, redirect] = ctrl_res_login(std::move(data));
            auto r = make_login_response(res);
            if (r.is_success()) {

                CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] " << r.get_response_name());

                //
                //	update state
                //
                state_ = state::AUTHORIZED;

                //
                //	set watchdog
                //
                if (watchdog != 0) {
                    CYNG_LOG_INFO(logger_, "ipt [" << tgl_.get() << "] set watchdog: " << watchdog << " minutes");

                    //
                    //	ToDo:
                    //
                }

                //
                //	signal changed authorization state
                //
                cb_auth_(true);
            } else {
                CYNG_LOG_WARNING(logger_, "ipt [" << tgl_.get() << "] login failed: " << r.get_response_name());
                boost::system::error_code ignored_ec;
                socket_.close(ignored_ec);
            }
        }

        void bus::register_target(std::string name, cyng::channel_weak wp) {

            boost::asio::post(dispatcher_, [this, name, wp]() {
                bool const b = buffer_write_.empty();
                auto r = serializer_.req_register_push_target(
                    name, std::numeric_limits<std::uint16_t>::max() //	0xffff
                    ,
                    1);

                //
                //	send register command to ip-t server
                //
                registrant_.emplace(r.second, std::make_pair(name, wp));
                buffer_write_.push_back(r.first);

                //
                //	send request
                //
                if (b)
                    do_write();
            });
        }

        void bus::open_channel(push_channel pcc) {

            boost::asio::post(dispatcher_, [this, pcc]() {
                bool const b = buffer_write_.empty();
                auto r =
                    serializer_.req_open_push_channel(pcc.target_, pcc.account_, pcc.number_, pcc.version_, pcc.id_, pcc.timeout_);

                //
                //	send "open push channel" command to ip-t server
                //
                buffer_write_.push_back(r.first);

                //
                //	send request
                //
                if (b)
                    do_write();
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
                auto const pos = registrant_.find(h.sequence_);
                if (pos != registrant_.end()) {

                    CYNG_LOG_INFO(
                        logger_, "[ipt] cmd " << ipt::command_name(h.command_) << ": "
                                              << ctrl_res_register_target_policy::get_response_name(res)
                                              << ", channel: " << channel);

                    //	name/channel
                    auto sp = pos->second.second.lock();
                    if (sp) {
                        if (ctrl_res_register_target_policy::is_success(res)) {
                            targets_.emplace(channel, std::move(pos->second));
                        } else {
                            //
                            //	stop task
                            //
                            sp->stop();
                        }
                    }

                    //
                    //	cleanup list
                    //
                    registrant_.erase(pos);
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << ": missing registrant");
                }
            });
        }

        void bus::pushdata_transfer(header const &h, cyng::buffer_t &&body) {

            /**
             * @return channel, source, status, block and data
             */
            auto [channel, source, status, block, data] = tp_req_pushdata_transfer(std::move(body));

            //
            //	sync with strand
            //
            boost::asio::post(dispatcher_, [this, h, channel, source, status, block, data]() {
                auto const pos = targets_.find(channel);
                if (pos != targets_.end()) {
                    CYNG_LOG_TRACE(
                        logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << channel << ':' << source << " - "
                                              << data.size() << " bytes");
                    //	name/channel
                    auto sp = pos->second.second.lock();
                    if (sp) {
                        sp->dispatch("receive", cyng::make_tuple(channel, source, data, pos->second.first));
                    } else {
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
        }

        void bus::req_watchdog(header const &h, cyng::buffer_t &&body) {
            BOOST_ASSERT(body.empty());
            CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " " << +h.sequence_);
            send(serializer_.res_watchdog(h.sequence_));
        }

        void bus::open_connection(std::string number, sequence_t seq) {

            if (state_ == state::AUTHORIZED) {
                state_ = state::LINKED;
                CYNG_LOG_TRACE(logger_, "[ipt] linked to " << number);
                send(serializer_.res_open_connection(seq, tp_res_open_connection_policy::DIALUP_SUCCESS));
            } else {
                send(serializer_.res_open_connection(seq, tp_res_open_connection_policy::BUSY));
            }
        }

        void bus::close_connection(sequence_t seq) {
            if (state_ == state::LINKED) {
                state_ = state::AUTHORIZED;
                send(serializer_.res_close_connection(seq, tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED));
                CYNG_LOG_TRACE(logger_, "[ipt] unlinked");
            } else {
                send(serializer_.res_close_connection(seq, tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED));
            }
        }

    } // namespace ipt
} // namespace smf
