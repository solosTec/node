#include <smf/ipt/bus.h>

#include <smf.h>
#include <smf/ipt/codes.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_format.h>
#include <smf/ipt/transpiler.h>

#include <cyng/log/record.h>
#include <cyng/net/client_factory.hpp>
#include <cyng/task/channel.h>
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
            cyng::controller &ctl,
            cyng::logger logger,
            toggle::server_vec_t &&tgl,
            std::string model,
            parser::command_cb cb_cmd,
            parser::data_cb cb_stream,
            auth_cb cb_auth)
            : state_(state::INITIAL)
            , ctl_(ctl)
            , logger_(logger)
            , tgl_(std::move(tgl))
            , client_()
            , model_(model)
            , cb_cmd_(cb_cmd)
            , cb_auth_(cb_auth)
            , serializer_(tgl_.get().sk_)
            , parser_(tgl_.get().sk_, std::bind(&bus::cmd_complete, this, std::placeholders::_1, std::placeholders::_2), cb_stream)
            , pending_targets_()
            , targets_()
            , opening_channel_()
            , channels_()
            , closing_channels_()
            , local_endpoint_()
            , remote_endpoint_() {

            cyng::net::client_factory cf(ctl);
            client_ = cf.create_proxy<boost::asio::ip::tcp::socket, 2048>(
                //  handle dispatch errors
                [this](std::string task, std::string slot) { CYNG_LOG_FATAL(logger_, task << " has no slot " << slot); },
                [this](std::size_t, std::size_t counter, std::string &host, std::string &service)
                    -> std::pair<std::chrono::seconds, bool> {
                    //
                    //  connect failed, reconnect after 20 seconds
                    //
                    state_ = state::INITIAL;
                    CYNG_LOG_WARNING(logger_, "[ipt] " << tgl_.get() << " connect failed for the " << counter << " time");
                    if (counter < 3) {
                        //  try same address again
                        return {std::chrono::seconds(20 * counter), true};
                    }
                    //  next redundancy
                    tgl_.changeover();
                    host = tgl_.get().host_;
                    service = tgl_.get().service_;
                    return {std::chrono::seconds(60), true};
                },
                [=, this](boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep, cyng::channel_ptr sp) {
                    CYNG_LOG_INFO(logger_, "[ipt] " << tgl_.get() << " connected to " << rep);

                    local_endpoint_ = lep;
                    remote_endpoint_ = rep;

                    //
                    //	send login sequence direct
                    //
                    state_ = state::CONNECTED;
                    auto const srv = tgl_.get();
                    if (srv.scrambled_) {

                        //
                        //	use a random key
                        //
                        auto const sk = gen_random_sk();

                        CYNG_LOG_INFO(logger_, "[ipt/" << tgl_.get() << "] sk = " << ipt::to_string(sk));
                        parser_.set_sk(sk);
                        client_.send(serializer_.req_login_scrambled(srv.account_, srv.pwd_, sk), true);
                    } else {
                        client_.send(serializer_.req_login_public(srv.account_, srv.pwd_), true);
                    }
                },
                [&](cyng::buffer_t data) {
                    //  read from socket
                    CYNG_LOG_DEBUG(logger_, "[ipt] " << tgl_.get() << " received " << data.size() << " bytes");

                    //
                    //	let parse it
                    //
                    parser_.read(data.begin(), data.end());
                },
                [=, this](boost::system::error_code ec) {
                    //
                    //	call disconnect function
                    //	signal changed authorization state
                    //
                    state_ = state::STOPPED;
                    CYNG_LOG_WARNING(logger_, "[ipt] connection closed: " << ec.message());
                    auto const ep = boost::asio::ip::tcp::endpoint();
                    cb_auth_(false, local_endpoint_, remote_endpoint_);

                    //
                    //  reconnect
                    //
                    tgl_.changeover();
                    client_.connect(tgl_.get().host_, tgl_.get().service_);
                },
                [this](cyng::net::client_state state) -> void {
                    //
                    //  update client state
                    //
                    switch (state) {
                    case cyng::net::client_state::INITIAL: CYNG_LOG_TRACE(logger_, "[ipt] client state INITIAL"); break;
                    case cyng::net::client_state::WAIT: CYNG_LOG_TRACE(logger_, "[ipt] client state WAIT"); break;
                    case cyng::net::client_state::CONNECTED: CYNG_LOG_TRACE(logger_, "[ipt] client state CONNECTED"); break;
                    case cyng::net::client_state::STOPPED: CYNG_LOG_TRACE(logger_, "[ipt] client state STOPPED"); break;
                    default: CYNG_LOG_ERROR(logger_, "[ipt] client state ERROR"); break;
                    }
                });
        }

        bool bus::is_authorized() const {
            //  at least authorized
            return state_ == state::AUTHORIZED || state_ == state::LINKED;
        }

        void bus::start() {

            CYNG_LOG_INFO(logger_, "[ipt] start client [" << tgl_.get() << "]");

            //
            //	connect to IP-T server
            //
            client_.connect(tgl_.get().host_, tgl_.get().service_);
        }

        void bus::stop() {
            // CYNG_LOG_INFO(logger_, "ipt " << tgl_.get() << " stop");
            client_.stop();
        }

        void bus::reset() {
            if (state_ != state::STOPPED) {

                pending_targets_.clear();
                targets_.clear();
                opening_channel_.clear();
                parser_.clear();

                if (is_authorized()) {

                    //
                    //	signal changed authorization state
                    //
                    local_endpoint_ = boost::asio::ip::tcp::endpoint();
                    remote_endpoint_ = boost::asio::ip::tcp::endpoint();
                    cb_auth_(false, local_endpoint_, remote_endpoint_);
                }
            }
        }

        void bus::cmd_complete(header const &h, cyng::buffer_t &&body) {

            // BOOST_ASSERT(state_holder_);
            // BOOST_ASSERT(!state_holder_->is_stopped());

            CYNG_LOG_TRACE(logger_, "[ipt/" << tgl_.get() << "] cmd " << command_name(h.command_));

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
            case code::APP_REQ_PROTOCOL_VERSION: client_.send(serializer_.res_protocol_version(h.sequence_, 1), false); break;
            case code::APP_REQ_SOFTWARE_VERSION:
                client_.send(serializer_.res_software_version(h.sequence_, SMF_VERSION_NAME), false);
                break;
            case code::APP_REQ_DEVICE_IDENTIFIER: client_.send(serializer_.res_device_id(h.sequence_, model_), false); break;
            case code::APP_REQ_DEVICE_AUTHENTIFICATION:
                client_.send(
                    serializer_.res_device_auth(
                        h.sequence_,
                        tgl_.get().account_,
                        tgl_.get().pwd_,
                        tgl_.get().account_, //	number
                        model_),
                    false);
                break;
            case code::APP_REQ_DEVICE_TIME: client_.send(serializer_.res_device_time(h.sequence_), false); break;
            case code::CTRL_RES_LOGIN_PUBLIC: res_login(std::move(body)); break;
            case code::CTRL_RES_LOGIN_SCRAMBLED:
                res_login(std::move(body));
                break;

                // case code::CTRL_REQ_LOGOUT:
                //	break;
                // case code::CTRL_RES_LOGOUT:
                //	break;

            case code::CTRL_REQ_REGISTER_TARGET:
                client_.send(serializer_.res_unknown_command(h.sequence_, h.command_), false);
                break;
            case code::CTRL_RES_REGISTER_TARGET: res_register_target(h, std::move(body)); break;
            case code::CTRL_REQ_DEREGISTER_TARGET:
                client_.send(serializer_.res_unknown_command(h.sequence_, h.command_), false);
                break;
            case code::CTRL_RES_DEREGISTER_TARGET: cb_cmd_(h, std::move(body)); break;
            case code::CTRL_REQ_WATCHDOG: req_watchdog(h, std::move(body)); break;

            case code::UNKNOWN:
            default: client_.send(serializer_.res_unknown_command(h.sequence_, h.command_), false); break;
            }
        }

        void bus::res_login(cyng::buffer_t &&data) {
            auto const [ok, res, watchdog, redirect] = ctrl_res_login(std::move(data));
            auto r = make_login_response(res);
            if (r.is_success()) {

                CYNG_LOG_INFO(logger_, "[ipt/" << tgl_.get() << "] " << r.get_response_name());

                //
                //	update state
                //
                state_ = state::AUTHORIZED;

                //
                //	set watchdog
                //
                if (watchdog != 0) {
                    CYNG_LOG_INFO(logger_, "[ipt/" << tgl_.get() << "] set watchdog: " << watchdog << " minutes");

                    //
                    //	ToDo: start watchdog
                    //
                }

                //
                //	signal changed authorization state
                //
                cb_auth_(true, local_endpoint_, remote_endpoint_);
            }
        }

        bool bus::register_target(std::string name, cyng::channel_weak wp) {

            if (!name.empty() && wp.lock()) {

                //
                //	send request
                //
                auto fn = [=, this]() -> cyng::buffer_t {
                    //  std::pair<cyng::buffer_t, sequence_t>
                    auto const [buffer, seq] = serializer_.req_register_push_target(
                        name,
                        std::numeric_limits<std::uint16_t>::max(), // max. packet size is 0xffff
                        1);                                        // window size is always 1

                    //
                    //	send register command to ip-t server
                    //
                    pending_targets_.emplace(seq, std::make_pair(name, wp));
                    CYNG_LOG_DEBUG(logger_, "[ipt] register target " << name << " with seq " << +seq);

                    //  buffer
                    return buffer;
                };
                client_.send(fn(), false);
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
                auto fn = [=, this]() -> cyng::buffer_t {
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
                };
                client_.send(fn(), false);
                return true;
            } else {
                CYNG_LOG_WARNING(logger_, "[ipt] cannot open push chanel: name or channel is empty");
            }
            return false;
        }

        bool bus::close_channel(std::uint32_t channel, cyng::channel_weak wp) {
            BOOST_ASSERT(!wp.expired());

            if (!wp.expired()) {
                auto fn = [=, this]() -> cyng::buffer_t {
                    CYNG_LOG_INFO(logger_, "[ipt] close channel: " << channel);

                    BOOST_ASSERT_MSG(state_ == state::AUTHORIZED || state_ == state::LINKED, "AUTHORIZED state expected");
                    auto const r = serializer_.req_close_push_channel(channel);

                    //
                    //  update list of pending channel closings
                    //
                    closing_channels_.emplace(r.second, std::make_pair(channel, wp));

                    //
                    //	send "close push channel" command to ip-t server
                    //
                    return r.first;
                };
                client_.send(fn(), false);
                return true;
            }

            return false;
        }

        void bus::res_open_push_channel(header const &h, cyng::buffer_t &&body) {

            //
            //	read message body (TP_RES_OPEN_CONNECTION)
            //
            auto const [ok, res, channel, source, packet_size, window_size, status, count] =
                tp_res_open_push_channel(std::move(body));

            BOOST_ASSERT(client_.get_channel());
            cyng::exec(client_.get_channel(), [this, h, res, channel, source, packet_size, window_size, status, count]() {
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
                            //  handle dispatch errors
                            std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
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
            auto const [ok, res, channel] = tp_res_close_push_channel(std::move(body));

            BOOST_ASSERT(client_.get_channel());

            cyng::exec(client_.get_channel(), [this, h, res, channel]() {
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
                        sp->dispatch(
                            "on.channel.close",
                            //  handle dispatch errors
                            std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                            tp_res_close_push_channel_policy::is_success(res),
                            channel);
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
            auto const [ok, res, channel] = ctrl_res_register_target(std::move(body));

            //
            //	sync with strand
            //
            BOOST_ASSERT(client_.get_channel());

            cyng::exec(client_.get_channel(), [this, h, res, channel]() {
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
                    CYNG_LOG_ERROR(
                        logger_,
                        "[ipt] cmd " << ipt::command_name(h.command_) << ": missing registrant for sequence " << +h.sequence_
                                     << " (channel = " << channel << ")");
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
            auto [ok, channel, source, status, block, data] = tp_req_pushdata_transfer(std::move(body));
            CYNG_LOG_TRACE(
                logger_,
                "[ipt] cmd " << ipt::command_name(h.command_) << " " << channel << ':' << source << " - " << data.size()
                             << " bytes");

            //
            //	sync with strand
            //
            BOOST_ASSERT(client_.get_channel());

            cyng::exec(client_.get_channel(), [this, h, channel, source, status, block, data]() {
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
                        //  handle dispatch errors
                        sp->dispatch(
                            "pushdata.transfer",
                            std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                            channel,
                            source,
                            data,
                            target);
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
            client_.send(serializer_.res_watchdog(h.sequence_), false);
        }

        void bus::open_connection(std::string number, sequence_t seq) {
            if (state_ == state::AUTHORIZED) {
                state_ = state::LINKED;
                CYNG_LOG_TRACE(logger_, "[ipt] linked to " << number);
                client_.send(serializer_.res_open_connection(seq, tp_res_open_connection_policy::DIALUP_SUCCESS), false);
            } else {
                client_.send(serializer_.res_open_connection(seq, tp_res_open_connection_policy::BUSY), false);
            }
        }

        void bus::close_connection(sequence_t seq) {
            if (state_ == state::LINKED) {
                state_ = state::AUTHORIZED;
                client_.send(
                    serializer_.res_close_connection(seq, tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED), false);
                CYNG_LOG_TRACE(logger_, "[ipt] unlinked");
            } else {
                client_.send(
                    serializer_.res_close_connection(seq, tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED), false);
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

            // send(state_holder_, std::bind(&serializer::req_transfer_push_data, &serializer_, id.first, id.second, 0xc1, 0,
            // data));
            client_.send(serializer_.req_transfer_push_data(id.first, id.second, 0xc1, 0, data), false);
        }

        void bus::transfer(cyng::buffer_t &&data) {
            CYNG_LOG_TRACE(logger_, "[ipt] transfer " << data.size() << " bytes");
            //  escape and scramble data
            client_.send(serializer_.escape_data(std::move(data)), false);
        }

        // bus::state::state(boost::asio::ip::tcp::resolver::results_type &&res)
        //     : value_(state_value::START)
        //     , endpoints_(std::move(res)) {}

        bool is_null(channel_id const &id) { return id == std::make_pair(0u, 0u); }
        void init(channel_id &id) { id = std::make_pair(0u, 0u); }

    } // namespace ipt
} // namespace smf
