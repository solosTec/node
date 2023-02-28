/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <session.h>
#include <tasks/gatekeeper.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/query.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_format.h>
#include <smf/ipt/transpiler.h>
#include <smf/obis/conv.h>
#include <smf/sml.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/parse/buffer.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>

#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serialize.h>
#include <iostream>
#include <sstream>
#ifdef _DEBUG_IPT
#include <smf/obis/db.h>
#include <smf/obis/list.h>
#include <smf/sml/reader.h>
#endif

namespace smf {

    ipt_session::ipt_session(
        boost::asio::ip::tcp::socket socket,
        cyng::logger logger,
        bus &cluster_bus,
        cyng::mesh &fabric,
        ipt::scramble_key const &sk,
        std::uint32_t query,
        cyng::mac48 client_id)
        : ctl_(fabric.get_ctl())
        , socket_(std::move(socket))
        , logger_(logger)
        , cluster_bus_(cluster_bus)
        , query_(query)
        , buffer_()
        , rx_{0}
        , sx_{0}
        , px_{0}
        , buffer_write_()
        , parser_(
              sk,
              std::bind(&ipt_session::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&ipt_session::ipt_stream, this, std::placeholders::_1))
        , name_()
        , serializer_(sk)
        , vm_()
        , dev_(boost::uuids::nil_uuid())
        , oce_map_() //	store temporary data during connection establishment
        , gatekeeper_()
        , proxy_(logger, *this, cluster_bus_, client_id)
#ifdef _DEBUG_IPT
        , sml_parser_([this](
                          std::string trx,
                          std::uint8_t group_no,
                          std::uint8_t,
                          sml::msg_type type,
                          cyng::tuple_t msg,
                          std::uint16_t crc) {
            CYNG_LOG_DEBUG(logger_, "msg: " << trx << " - " << sml::get_name(type));
            switch (type) {
            case sml::msg_type::GET_PROC_PARAMETER_RESPONSE: {
                auto const [s, p, code, a, l] = sml::read_get_proc_parameter_response(msg);
                CYNG_LOG_DEBUG(logger_, "server: " << s);
                CYNG_LOG_DEBUG(logger_, "path  : " << p);
                CYNG_LOG_DEBUG(logger_, "code  : " << code << " - " << obis::get_name(code));
                CYNG_LOG_DEBUG(logger_, "attr  : " << a);
                CYNG_LOG_DEBUG(logger_, "list  : \n" << sml::dump_child_list(l, false));
            } break;
            case sml::msg_type::ATTENTION_RESPONSE: {
                auto const code = sml::read_attention_response(msg);
                CYNG_LOG_WARNING(
                    logger_, "[sml] " << sml::get_name(type) << ": " << trx << ", " << msg << " - " << obis::get_name(code));
            } break;
            default: CYNG_LOG_DEBUG(logger_, "\n" << cyng::io::to_pretty(msg)); break;
            }
        })
#endif
    {
        vm_ = fabric.make_proxy(
            cluster_bus_.get_tag(),
            cyng::make_description(
                "pty.res.login", cyng::vm_adaptor<ipt_session, void, bool, boost::uuids::uuid>(this, &ipt_session::pty_res_login)),

            cyng::make_description(
                "pty.res.register",
                cyng::vm_adaptor<ipt_session, void, bool, std::uint32_t, cyng::param_map_t>(this, &ipt_session::pty_res_register)),

            cyng::make_description(
                "pty.res.open.channel",
                cyng::vm_adaptor<
                    ipt_session,
                    void,
                    bool,
                    std::uint32_t,
                    std::uint32_t,
                    std::uint16_t,
                    std::uint8_t,
                    std::uint8_t,
                    std::uint32_t,
                    cyng::param_map_t>(this, &ipt_session::pty_res_open_channel)),

            cyng::make_description(
                "pty.req.push.data",
                cyng::vm_adaptor<ipt_session, void, std::uint32_t, std::uint32_t, cyng::buffer_t>(
                    this, &ipt_session::pty_req_push_data)),

            cyng::make_description(
                "pty.push.data.res",
                cyng::vm_adaptor<ipt_session, void, bool, std::uint32_t, std::uint32_t, cyng::param_map_t>(
                    this, &ipt_session::pty_res_push_data)),

            cyng::make_description(
                "pty.res.close.channel",
                cyng::vm_adaptor<ipt_session, void, bool, std::uint32_t, std::size_t, cyng::param_map_t>(
                    this, &ipt_session::pty_res_close_channel)),

            cyng::make_description(
                "pty.res.open.connection",
                cyng::vm_adaptor<ipt_session, void, bool, cyng::param_map_t>(this, &ipt_session::pty_res_open_connection)),

            cyng::make_description(
                "pty.transfer.data", cyng::vm_adaptor<ipt_session, void, cyng::buffer_t>(this, &ipt_session::pty_transfer_data)),

            cyng::make_description(
                "pty.res.close.connection",
                cyng::vm_adaptor<ipt_session, void, bool, cyng::param_map_t>(this, &ipt_session::pty_res_close_connection)),

            cyng::make_description(
                "pty.req.open.connection",
                cyng::vm_adaptor<ipt_session, void, std::string, bool, cyng::param_map_t>(
                    this, &ipt_session::pty_req_open_connection)),

            cyng::make_description(
                "pty.req.close.connection", cyng::vm_adaptor<ipt_session, void>(this, &ipt_session::pty_req_close_connection)),

            cyng::make_description("pty.req.stop", cyng::vm_adaptor<ipt_session, void>(this, &ipt_session::pty_stop)),

            cyng::make_description(
                "cfg.req.backup",
                cyng::vm_adaptor<
                    ipt_session,
                    void,
                    std::string,
                    std::string,
                    boost::uuids::uuid, //  key in table "gateway"
                    cyng::buffer_t,     //  server id
                    std::string,        //  firware version
                    std::chrono::system_clock::time_point>(this, &ipt_session::cfg_req_backup)),

            cyng::make_description(
                "cfg.sml.channel",
                cyng::vm_adaptor<
                    ipt_session,
                    void,
                    std::string,        //  operator_name,
                    std::string,        //  operator_pwd,
                    boost::uuids::uuid, //  dev_tag,
                    cyng::buffer_t,     //  id,
                    std::string,        // channel,
                    cyng::obis,         // section,
                    cyng::param_map_t,  // params,
                    boost::uuids::uuid, // source,
                    boost::uuids::uuid  // tag
                    >(this, &ipt_session::cfg_sml_channel)));

        CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << socket_.remote_endpoint() << " created");
    }

    ipt_session::~ipt_session() {
        // gatekeeper_->stop(); //  this is impossible
#ifdef _DEBUG_IPT
        // std::cout << "session(~)" << std::endl;
#endif
    }

    void ipt_session::stop() {
        //	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
        if (vm_.stop()) {

            //    no more write
            //  buffer_write_.clear();
            BOOST_ASSERT_MSG(buffer_write_.empty(), "pending write op");
            oce_map_.clear();

            //  this can cause to call the stop() function again
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
            socket_.close(ec);
        }
    }

    void ipt_session::logout() { cluster_bus_.pty_logout(dev_, vm_.get_tag()); }

    boost::asio::ip::tcp::endpoint ipt_session::get_remote_endpoint() const {
        boost::system::error_code ec;
        return socket_.remote_endpoint(ec);
    }

    void ipt_session::start(std::chrono::seconds timeout) {
        do_read();
        gatekeeper_ =
            ctl_.create_channel_with_ref<gatekeeper>(logger_, this->shared_from_this(), cluster_bus_, socket_.remote_endpoint())
                .first;
        BOOST_ASSERT(gatekeeper_->is_open());
        CYNG_LOG_TRACE(logger_, "start gatekeeper with a timeout of " << timeout.count() << " seconds");
        gatekeeper_->suspend(timeout, "timeout");
    }

    void ipt_session::do_read() {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    CYNG_LOG_DEBUG(
                        logger_,
                        "[session] " << vm_.get_tag() << " received " << bytes_transferred << " bytes from ["
                                     << socket_.remote_endpoint() << "]");

#ifdef __DEBUG_IPT
                    {
                        std::stringstream ss;
                        cyng::io::hex_dump<8> hd;
                        hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
                        auto const dmp = ss.str();

                        CYNG_LOG_DEBUG(
                            logger_,
                            "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes scrambled "
                                << ipt::to_string(parser_.get_sk()) << "@" << parser_.get_scrambler_index() << " ip-t data:\n"
                                << dmp);
                    }
#endif
                    //
                    //	let parse it
                    //
                    auto const data = parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
#ifdef _DEBUG_IPT
                    {
                        std::stringstream ss;
                        cyng::io::hex_dump<8> hd;
                        hd(ss, data.begin(), data.end());
                        auto const dmp = ss.str();
                        CYNG_LOG_DEBUG(
                            logger_,
                            "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes (unscrambled) ip-t data:\n"
                                << dmp);

                        //
                        //  debug sml data
                        //
                        // sml_parser_.read(std::begin(data), std::end(data));
                    }
#endif

                    //
                    //	update rx
                    //
                    rx_ += static_cast<std::uint64_t>(bytes_transferred);
                    if (!dev_.is_nil() && cluster_bus_.is_connected()) {

                        cluster_bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("rx", rx_));
                    }

                    //
                    //	continue reading
                    //
                    do_read();
                } else {
                    CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " read: " << ec.message());
                    stop();
                }
            });
    }

    void ipt_session::do_write() {
        BOOST_ASSERT_MSG(!buffer_write_.empty(), "write buffer empty");

#ifdef _DEBUG
        CYNG_LOG_DEBUG(
            logger_,
            "send ipt msg #" << buffer_write_.size() << ": " << buffer_write_.front().size() << " bytes to "
                             << socket_.remote_endpoint());
#endif

        // Start an asynchronous operation to send a heartbeat message.
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            cyng::expose_dispatcher(vm_).wrap(std::bind(&ipt_session::handle_write, this, std::placeholders::_1)));
    }

    void ipt_session::handle_write(const boost::system::error_code &ec) {

        if (!ec) {

            //
            //	update sx
            //
            sx_ += static_cast<std::uint64_t>(buffer_write_.front().size());
            if (!dev_.is_nil() && cluster_bus_.is_connected()) {

                cluster_bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("sx", sx_));
            }

            buffer_write_.pop_front();
            if (!buffer_write_.empty()) {
                do_write();
            }
        } else {
            CYNG_LOG_ERROR(logger_, "[session] " << vm_.get_tag() << " write: " << ec.message());

            // reset();
        }
    }

    void ipt_session::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {
        CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));

        switch (ipt::to_code(h.command_)) {
        case ipt::code::CTRL_REQ_LOGIN_PUBLIC:
            if (cluster_bus_.is_connected()) {
                std::string pwd;
                bool ok = false;
                std::tie(ok, this->name_, pwd) = ipt::ctrl_req_login_public(std::move(body));
                if (ok) {
                    CYNG_LOG_INFO(logger_, "[ipt] public login: " << name_ << ':' << pwd);
                    cluster_bus_.pty_login(name_, pwd, vm_.get_tag(), "sml", socket_.remote_endpoint());
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] invalid public login request");

                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, body.begin(), body.end());
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] <-- public login request:\n" << dmp);
                }
            } else {
                CYNG_LOG_WARNING(logger_, "[session] login rejected - MALFUNCTION");
                ipt_send(std::bind(
                    &ipt::serializer::res_login_public, &serializer_, ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
            }
            break;
        case ipt::code::CTRL_REQ_LOGIN_SCRAMBLED:
            if (cluster_bus_.is_connected()) {
                std::string pwd;
                ipt::scramble_key sk;
                bool ok = false;
                std::tie(ok, this->name_, pwd, sk) = ipt::ctrl_req_login_scrambled(std::move(body));
                if (ok) {
                    CYNG_LOG_INFO(logger_, "[ipt] scrambled login: " << name_ << ':' << pwd << ", sk = " << ipt::to_string(sk));
                    parser_.set_sk(sk);
                    serializer_.set_sk(sk);
                    cluster_bus_.pty_login(name_, pwd, vm_.get_tag(), "sml", socket_.remote_endpoint());
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] invalid scrambled login request");

                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, body.begin(), body.end());
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] <-- scrambled login request:\n" << dmp);
                }
            } else {
                CYNG_LOG_WARNING(logger_, "[session] login rejected - MALFUNCTION");
                ipt_send(std::bind(
                    &ipt::serializer::res_login_public, &serializer_, ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
            }
            break;
        case ipt::code::APP_RES_SOFTWARE_VERSION: update_software_version(ipt::app_res_software_version(std::move(body))); break;
        case ipt::code::APP_RES_DEVICE_IDENTIFIER: update_device_identifier(ipt::app_res_device_identifier(std::move(body))); break;
        case ipt::code::CTRL_REQ_REGISTER_TARGET:
            if (cluster_bus_.is_connected()) {
                auto const [ok, name, paket_size, window_size] = ipt::ctrl_req_register_target(std::move(body));
                if (ok) {
                    register_target(name, paket_size, window_size, h.sequence_);
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] invalid register request");

                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, body.begin(), body.end());
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] <-- register target request:\n" << dmp);
                }
            }
            break;
        case ipt::code::CTRL_REQ_DEREGISTER_TARGET:
            if (cluster_bus_.is_connected()) {
                auto const name = ipt::ctrl_req_deregister_target(std::move(body));
                deregister_target(name, h.sequence_);
            }
            break;
        case ipt::code::TP_REQ_OPEN_PUSH_CHANNEL:
            if (cluster_bus_.is_connected()) {
                auto const [ok, target, account, msisdn, version, id, timeout] = ipt::tp_req_open_push_channel(std::move(body));
                if (ok) {
                    open_push_channel(target, account, msisdn, version, id, timeout, h.sequence_);
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] invalid open push channel request");

                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, body.begin(), body.end());
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] <-- open push channel:\n" << dmp);
                }
            }
            break;
        case ipt::code::TP_REQ_CLOSE_PUSH_CHANNEL:
            if (cluster_bus_.is_connected()) {
                auto const channel = ipt::ctrl_req_close_push_channel(std::move(body));
                close_push_channel(channel, h.sequence_);
            }
            break;
        case ipt::code::TP_REQ_PUSHDATA_TRANSFER:
            if (cluster_bus_.is_connected()) {
                auto const [ok, channel, source, status, block, data] = ipt::tp_req_pushdata_transfer(std::move(body));
                if (ok) {
                    pushdata_transfer(channel, source, status, block, data, h.sequence_);
                } else {
                    CYNG_LOG_ERROR(logger_, "[ipt] invalid open push channel request");

                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, body.begin(), body.end());
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] <-- push data transfer:\n" << dmp);
                }
            }
            break;
        case ipt::code::TP_REQ_OPEN_CONNECTION:
            if (cluster_bus_.is_connected()) {
                auto const number = ipt::tp_req_open_connection(std::move(body));
                open_connection(number, h.sequence_);
            }
            break;
        case ipt::code::TP_RES_OPEN_CONNECTION:
            if (cluster_bus_.is_connected()) {
                auto const res = ipt::tp_res_open_connection(std::move(body));
                CYNG_LOG_INFO(
                    logger_, "[ipt] response open connection: " << ipt::tp_res_open_connection_policy::get_response_name(res));
                CYNG_LOG_TRACE(
                    logger_, "[ipt] connect success: " << (ipt::tp_res_open_connection_policy::is_success(res) ? "yes" : " no"));

                //
                //	search for temporary connection establishment data
                //
                auto const pos = oce_map_.find(h.sequence_);
                if (pos != oce_map_.end()) {

                    auto const reader = cyng::make_reader(pos->second);
                    auto const caller_tag = cyng::value_cast(reader["caller-tag"].get(), boost::uuids::nil_uuid());
                    auto const caller_vm = cyng::value_cast(reader["caller-peer"].get(), boost::uuids::nil_uuid());
                    BOOST_ASSERT(!caller_tag.is_nil());
                    BOOST_ASSERT(!caller_vm.is_nil());

                    CYNG_LOG_TRACE(logger_, "[ipt] forward response to " << caller_tag << " on vm:" << caller_vm);

                    cluster_bus_.pty_res_open_connection(
                        ipt::tp_res_open_connection_policy::is_success(res), dev_, vm_.get_tag(), std::move(pos->second));

                    //
                    //	cleanup oce map
                    //
                    oce_map_.erase(pos);

                } else {
                    CYNG_LOG_WARNING(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " without request");
                }
            } else {
                //  close connection
                ipt_send(std::bind(&ipt::serializer::req_close_connection, &serializer_));
            }

            break;
        case ipt::code::TP_RES_OPEN_PUSH_CHANNEL:
            //
            //	open push channel response is mostly an error when received from ip-t server
            //
            // 	Normally TP_RES_OPEN_PUSH_CHANNEL should not be used, because the open
            //	push channel request is answered by the IP-T server.
            //	There is a bug in the VARIOSafe Manager to answer an open
            //	connection request with an open push channel response.
            //
            if (cluster_bus_.is_connected()) {
                if (body.size() < 20) {
                    CYNG_LOG_ERROR(
                        logger_, "[ipt] invalid response open push channel - only " << body.size() << " bytes available");
                } else {
                    auto const [ok, res, channel, source, ps, ws, status, count] = ipt::tp_res_open_push_channel(std::move(body));
                    CYNG_LOG_WARNING(
                        logger_,
                        "[ipt] response open push channel " << channel << ':' << source << ": "
                                                            << ipt::tp_res_open_push_channel_policy::get_response_name(res));
                }
            }
            break;
        case ipt::code::UNKNOWN:
            CYNG_LOG_WARNING(logger_, "[ipt] unknown command " << ipt::ctrl_res_unknown_cmd(std::move(body)));
            break;
        default:
            CYNG_LOG_WARNING(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " ignored");
            // ipt_send(std::bind(&ipt::serializer::res_unknown_command, &serializer_, h.sequence_, h.command_));
            break;
        }
    }

    void ipt_session::register_target(std::string name, std::uint16_t paket_size, std::uint8_t window_size, ipt::sequence_t seq) {
        CYNG_LOG_INFO(logger_, "[ipt] register: " << name);
        cluster_bus_.pty_reg_target(name, paket_size, window_size, dev_, vm_.get_tag(), cyng::param_map_factory("seq", seq));
    }

    void ipt_session::deregister_target(std::string name, ipt::sequence_t seq) {
        CYNG_LOG_INFO(logger_, "[ipt] deregister: " << name);
        cluster_bus_.pty_dereg_target(name, dev_, vm_.get_tag(), cyng::param_map_factory("seq", seq));
    }

    void ipt_session::open_push_channel(
        std::string name,
        std::string account,
        std::string msisdn,
        std::string version,
        std::string id,
        std::uint16_t timeout,
        ipt::sequence_t seq) {
        CYNG_LOG_INFO(logger_, "[ipt] open push channel: " << name);
        cluster_bus_.pty_open_channel(
            name,
            account,
            msisdn,
            version,
            id,
            std::chrono::seconds(timeout),
            dev_,
            vm_.get_tag(),
            cyng::param_map_factory("seq", seq));
    }

    void ipt_session::close_push_channel(std::uint32_t channel, ipt::sequence_t seq) {
        CYNG_LOG_INFO(logger_, "[ipt] close push channel: " << channel);
        cluster_bus_.pty_close_channel(channel, dev_, vm_.get_tag(), cyng::param_map_factory("seq", seq));
    }

    void ipt_session::pushdata_transfer(
        std::uint32_t channel,
        std::uint32_t source,
        std::uint8_t status,
        std::uint8_t block,
        cyng::buffer_t data,
        ipt::sequence_t seq) {
        auto const size = data.size();

        CYNG_LOG_INFO(logger_, "[ipt] pushdata transfer " << channel << ':' << source << ", " << size << " bytes");
        cluster_bus_.pty_push_data(
            channel,
            source,
            std::move(data),
            dev_,
            vm_.get_tag(),
            cyng::param_map_factory("seq", seq)("status", status)("block", block));

        //
        //	update px
        //
        px_ += static_cast<std::uint64_t>(size);
        if (!dev_.is_nil() && cluster_bus_.is_connected()) {

            cluster_bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("px", px_));
        }
    }

    void ipt_session::open_connection(std::string msisdn, ipt::sequence_t seq) {
        CYNG_LOG_INFO(logger_, "[ipt] open connection: " << msisdn);
        cluster_bus_.pty_open_connection(msisdn, dev_, vm_.get_tag(), cyng::param_map_factory("seq", seq));
    }

    void ipt_session::update_software_version(std::string str) {
        CYNG_LOG_INFO(logger_, "[ipt] software version: " << str);

        //
        //	update "device" table
        //
        cyng::param_map_t const data = cyng::param_map_factory()("vFirmware", str);
        cluster_bus_.req_db_update("device", cyng::key_generator(dev_), data);
    }
    void ipt_session::update_device_identifier(std::string str) {
        CYNG_LOG_INFO(logger_, "[ipt] device id: " << str);

        //
        //	update "device" table
        //  "main" node will this device insert as gateway if string starts with "swf-gw:"
        //
        cyng::param_map_t const data = cyng::param_map_factory()("id", str);
        cluster_bus_.req_db_update("device", cyng::key_generator(dev_), data);
    }

    void ipt_session::ipt_stream(cyng::buffer_t &&data) {
#ifdef _DEBUG_IPT
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(
                logger_,
                "received " << data.size() << " stream bytes from[" << socket_.remote_endpoint() << "]:\n"
                            << dmp);
            //
            //   debug sml data
            //
            // CYNG_LOG_DEBUG(logger_, "received " << data.size() << " stream bytes from [" << socket_.remote_endpoint() << "]");
            sml_parser_.read(std::begin(data), std::end(data));
        }
#endif
        //  redirect data optionally to proxy
        if (cluster_bus_.is_connected()) {
            if (proxy_.is_online()) {
                CYNG_LOG_TRACE(logger_, "routing " << data.size() << " bytes to proxy");
                proxy_.read(std::move(data));
            } else {
                CYNG_LOG_TRACE(logger_, "ipt stream " << data.size() << " byte");
                cluster_bus_.pty_transfer_data(dev_, vm_.get_tag(), std::move(data));
            }
        } else {
            CYNG_LOG_WARNING(logger_, "ipt stream " << data.size() << " byte");
        }
    }

    void ipt_session::ipt_send(std::function<cyng::buffer_t()> f) {
        cyng::exec(vm_, [this, f]() {
            bool const b = buffer_write_.empty();
            buffer_write_.push_back(f());
            if (b) {
                do_write();
            }
        });
    }

    void ipt_session::pty_res_login(bool success, boost::uuids::uuid dev) {

        if (success) {

            //
            //	stop gatekeeper
            //
            gatekeeper_->stop();

            //
            //	update device tag
            //
            dev_ = dev;

            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " login ok");
            ipt_send(
                std::bind(&ipt::serializer::res_login_public, &serializer_, ipt::ctrl_res_login_public_policy::SUCCESS, 0, ""));

            query();

        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
            ipt_send(std::bind(
                &ipt::serializer::res_login_public, &serializer_, ipt::ctrl_res_login_public_policy::UNKNOWN_ACCOUNT, 0, ""));

            //
            // Don't call stop here. This would disrupt the async write op.
            // The gatekeeper will stop this session.
            // stop();
            //
        }
    }

    void ipt_session::pty_res_register(bool success, std::uint32_t channel, cyng::param_map_t token) {
        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " register target ok: " << token);

            ipt_send(std::bind(
                &ipt::serializer::res_register_push_target, &serializer_, seq, ipt::ctrl_res_register_target_policy::OK, channel));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " register target failed: " << token);
            ipt_send(std::bind(
                &ipt::serializer::res_register_push_target,
                &serializer_,
                seq,
                ipt::ctrl_res_register_target_policy::REJECTED,
                channel));
        }
    }

    void ipt_session::pty_res_open_channel(
        bool success,
        std::uint32_t channel,
        std::uint32_t source,
        std::uint16_t packet_size,
        std::uint8_t window_size,
        std::uint8_t status,
        std::uint32_t count,
        cyng::param_map_t token) {
        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(
                logger_, "[pty] " << vm_.get_tag() << " open push channel " << channel << ':' << source << " ok: " << token);

            BOOST_ASSERT(channel != 0);
            BOOST_ASSERT(window_size == 1);
            BOOST_ASSERT(count != 0);

            ipt_send(std::bind(
                &ipt::serializer::res_open_push_channel,
                &serializer_,
                seq,
                ipt::tp_res_open_push_channel_policy::SUCCESS,
                channel,
                source,
                packet_size,
                window_size,
                status,
                count));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " open push channel failed: " << token);
            ipt_send(std::bind(
                &ipt::serializer::res_open_push_channel,
                &serializer_,
                seq,
                ipt::tp_res_open_push_channel_policy::UNREACHABLE,
                0,
                0,
                0,
                1,
                0,
                0));
        }
    }

    void ipt_session::pty_req_push_data(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data) {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " push data " << data.size() << " bytes");
#ifdef _DEBUG_IPT
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] --> outgoing push data:\n" << dmp);
        }
#endif
        std::uint8_t status = 0;
        std::uint8_t block = 0;
        ipt_send(std::bind(&ipt::serializer::req_transfer_push_data, &serializer_, channel, source, status, block, data));
    }

    void ipt_session::pty_res_push_data(bool success, std::uint32_t channel, std::uint32_t source, cyng::param_map_t token) {
        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);
        auto const status = cyng::value_cast<std::uint8_t>(reader["status"].get(), 0);
        auto const block = cyng::value_cast<std::uint8_t>(reader["block"].get(), 0);

        //	%(("block":0),("seq":6d),("status":c1))
        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " push data " << channel << " ok: " << token);
            if (status != 0xc1) {
                CYNG_LOG_WARNING(
                    logger_,
                    "[pty] " << vm_.get_tag() << " push data " << channel << ':' << source
                             << " invalid push channel status: " << +status);
            }

            ipt_send(std::bind(
                &ipt::serializer::res_transfer_push_data,
                &serializer_,
                seq,
                ipt::tp_res_pushdata_transfer_policy::SUCCESS,
                channel,
                source,
                status | ipt::tp_res_pushdata_transfer_policy::ACK,
                block));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " push data " << channel << " failed: " << token);
            ipt_send([=, this]() {
                return serializer_.res_transfer_push_data(
                    seq, ipt::tp_res_pushdata_transfer_policy::UNREACHABLE, channel, source, status, block);
            });
        }
    }

    void ipt_session::pty_res_close_channel(bool success, std::uint32_t channel, std::size_t count, cyng::param_map_t token) {
        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " close push channel " << channel << " ok: " << token);
            ipt_send(std::bind(
                &ipt::serializer::res_close_push_channel,
                &serializer_,
                seq,
                ipt::tp_res_close_push_channel_policy::SUCCESS,
                channel));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " close push channel " << channel << " failed: " << token);
            ipt_send(std::bind(
                &ipt::serializer::res_close_push_channel,
                &serializer_,
                seq,
                ipt::tp_res_close_push_channel_policy::BROKEN,
                channel));
        }
    }

    void ipt_session::pty_res_open_connection(bool success, cyng::param_map_t token) {
        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);

        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " dialup ok: " << token);
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " dialup failed: " << token);
        }

        ipt_send(std::bind(
            &ipt::serializer::res_open_connection,
            &serializer_,
            seq,
            success ? ipt::tp_res_open_connection_policy::DIALUP_SUCCESS : ipt::tp_res_open_connection_policy::DIALUP_FAILED));
    }

    void ipt_session::pty_res_close_connection(bool success, cyng::param_map_t token) {
        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed " << token);

        BOOST_ASSERT(seq != 0);
        ipt_send(std::bind(
            &ipt::serializer::res_close_connection,
            &serializer_,
            seq,
            success ? ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED
                    : ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED));
    }

    void ipt_session::pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token) {
        ipt_send([=, this]() -> cyng::buffer_t {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " req open connection " << msisdn);
            auto r = serializer_.req_open_connection(msisdn);
            oce_map_.emplace(r.second, token);
            return r.first;
        });
    }

    void ipt_session::pty_transfer_data(cyng::buffer_t data) {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " transfer " << data.size() << " bytes to gateway");
#ifdef _DEBUG_IPT
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] emit " << data.size() << " bytes:\n" << dmp);
        }

        if (boost::algorithm::equals(std::string(data.begin(), data.end()), "hello")) {
            std::string const answer("welcome!\r\n");
            cluster_bus_.pty_transfer_data(dev_, vm_.get_tag(), cyng::buffer_t(answer.begin(), answer.end()));
        }
#endif
        //
        //  send data to device/gateway
        //  escape and scramble data
        //
        ipt_send([=, this]() mutable -> cyng::buffer_t { return serializer_.escape_data(std::move(data)); });
    }

    void ipt_session::pty_req_close_connection() {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed");
        ipt_send(std::bind(&ipt::serializer::req_close_connection, &serializer_));
    }

    void ipt_session::pty_stop() {
        CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " stop");
        stop();
    }

    void ipt_session::cfg_req_backup(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        cyng::buffer_t id,
        std::string fw, //  firware version
        std::chrono::system_clock::time_point tp) {
        //
        //  start SML proxy
        //
        proxy_.cfg_backup(name, pwd, tag, id, fw, tp);
    }

    void ipt_session::cfg_sml_channel(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        cyng::buffer_t id,
        std::string channel,
        cyng::obis section,
        cyng::param_map_t params,
        boost::uuids::uuid source,
        boost::uuids::uuid tag_cluster) {

        CYNG_LOG_INFO(logger_, "[cfg] channel: " << channel << ", section: " << section);
        //  convert section to OBIS
        // auto const code = cyng::make_obis(cyng::hex_to_buffer(section));

        //  get a path
        auto const reader = cyng::make_reader(params);
        auto const vec = cyng::vector_cast<std::string>(reader["path"].get(), "");

        auto const type = sml::msg_type_by_name(channel);

        switch (type) {
        case sml::msg_type::GET_PROC_PARAMETER_REQUEST:
            //
            //  GET_PROC_PARAMETER_REQUEST
            //
            proxy_.get_proc_param_req(
                name,
                pwd,
                tag,
                id,
                section,
                source,
                vec.empty() ? cyng::obis_path_t({section}) : obis::to_obis_path(vec),
                tag_cluster);
            break;
        case sml::msg_type::SET_PROC_PARAMETER_REQUEST:
            //
            //  SET_PROC_PARAMETER_REQUEST
            //
            proxy_.set_proc_param_req(
                name,
                pwd,
                tag,
                id,
                section,
                params,
                source,
                vec.empty() ? cyng::obis_path_t({section}) : obis::to_obis_path(vec),
                tag_cluster);
            break;
        case sml::msg_type::GET_PROFILE_LIST_REQUEST:
            //
            //  GET_LIST_REQUEST
            //
            proxy_.get_profile_list_req(name, pwd, tag, id, section, params, source, tag_cluster);
            break;
        // case sml::msg_type::GET_LIST_REQUEST:
        //     //
        //     //  GET_LIST_REQUEST
        //     //
        //     proxy_.get_list_req(name, pwd, tag, id, code, params, source, tag_cluster);
        //     break;
        default: CYNG_LOG_WARNING(logger_, "[cfg] unknown channel: " << channel << ", section: " << section); break;
        }
    }

    void ipt_session::query() {
        if (ipt::test_bit(query_, ipt::query::PROTOCOL_VERSION)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::PROTOCOL_VERSION)));
            ipt_send(std::bind(&ipt::serializer::req_protocol_version, &serializer_));
        }
        if (ipt::test_bit(query_, ipt::query::FIRMWARE_VERSION)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::FIRMWARE_VERSION)));
            ipt_send(std::bind(&ipt::serializer::req_software_version, &serializer_));
        }
        if (ipt::test_bit(query_, ipt::query::DEVICE_IDENTIFIER)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_IDENTIFIER)));
            ipt_send(std::bind(&ipt::serializer::req_device_id, &serializer_));
        }
        if (ipt::test_bit(query_, ipt::query::NETWORK_STATUS)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::NETWORK_STATUS)));
            ipt_send(std::bind(&ipt::serializer::req_network_status, &serializer_));
        }
        if (ipt::test_bit(query_, ipt::query::IP_STATISTIC)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::IP_STATISTIC)));
            ipt_send(std::bind(&ipt::serializer::req_ip_statistics, &serializer_));
        }
        if (ipt::test_bit(query_, ipt::query::DEVICE_AUTHENTIFICATION)) {
            CYNG_LOG_TRACE(
                logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_AUTHENTIFICATION)));
            ipt_send(std::bind(&ipt::serializer::req_device_auth, &serializer_));
        }
        if (ipt::test_bit(query_, ipt::query::DEVICE_TIME)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_TIME)));
            ipt_send(std::bind(&ipt::serializer::req_device_time, &serializer_));
        }
    }

    void ipt_session::update_connect_state(bool connected) {
        //
        //  build key for "connection" table
        //
        auto const key_conn = cyng::key_generator(cyng::merge(dev_, vm_.get_tag()));
        if (connected) {
            cluster_bus_.req_db_insert(
                "connection",
                key_conn,
                cyng::data_generator(
                    name_ + "-proxy",
                    name_,
                    std::chrono::system_clock::now(),
                    true,  //  local
                    "ipt", //  protocol layer
                    "ipt",
                    static_cast<std::uint64_t>(0)),
                1);
        } else {
            cluster_bus_.req_db_remove("connection", key_conn);
        }
    }

} // namespace smf
