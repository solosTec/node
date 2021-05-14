/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <ipt_session.h>
#include <tasks/gatekeeper.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/query.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/scramble_key_format.h>
#include <smf/ipt/transpiler.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>

#ifdef _DEBUG_IPT
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    ipt_session::ipt_session(
        boost::asio::ip::tcp::socket socket, bus &cluster_bus, cyng::mesh &fabric, ipt::scramble_key const &sk, std::uint32_t query,
        cyng::logger logger)
        : ctl_(fabric.get_ctl()),
          socket_(std::move(socket)),
          logger_(logger),
          cluster_bus_(cluster_bus),
          query_(query),
          buffer_(),
          rx_{0},
          sx_{0},
          px_{0},
          buffer_write_(),
          parser_(
              sk, std::bind(&ipt_session::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&ipt_session::ipt_stream, this, std::placeholders::_1)),
          serializer_(sk),
          vm_(),
          dev_(boost::uuids::nil_uuid()),
          oce_map_(), //	store temporary data during connection establishment
          gatekeeper_() {
        vm_ = fabric.create_proxy(
            cluster_bus_.get_tag(), get_vm_func_pty_res_login(this), get_vm_func_pty_res_register(this),
            get_vm_func_pty_res_open_channel(this), get_vm_func_pty_req_push_data(this), get_vm_func_pty_res_push_data(this),
            get_vm_func_pty_res_close_channel(this), get_vm_func_pty_res_open_connection(this), get_vm_func_pty_transfer_data(this),
            get_vm_func_pty_res_close_connection(this), get_vm_func_pty_req_open_connection(this),
            get_vm_func_pty_req_close_connection(this));

        std::size_t slot{0};
        vm_.set_channel_name("pty.res.login", slot++);        //	get_vm_func_pty_res_login
        vm_.set_channel_name("pty.res.register", slot++);     //	get_vm_func_pty_res_register
        vm_.set_channel_name("pty.res.open.channel", slot++); //	get_vm_func_pty_res_open_channel

        vm_.set_channel_name("pty.push.data.req", slot++); //	get_vm_func_pty_req_push_data
        vm_.set_channel_name("pty.push.data.res", slot++); //	get_vm_func_pty_res_push_data

        vm_.set_channel_name("pty.res.close.channel", slot++);    //	get_vm_func_pty_res_close_channel
        vm_.set_channel_name("pty.res.open.connection", slot++);  //	get_vm_func_pty_res_open_connection
        vm_.set_channel_name("pty.transfer.data", slot++);        //	get_vm_func_pty_transfer_data
        vm_.set_channel_name("pty.res.close.connection", slot++); //	get_vm_func_pty_res_close_connection
        vm_.set_channel_name("pty.req.open.connection", slot++);  //	get_vm_func_pty_req_open_connection
        vm_.set_channel_name("pty.req.close.connection", slot++); //	get_vm_func_pty_req_close_connection

        CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << socket_.remote_endpoint() << " created");
    }

    ipt_session::~ipt_session() {
#ifdef _DEBUG_IPT
        std::cout << "session(~)" << std::endl;
#endif
    }

    void ipt_session::stop() {
        //	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
        gatekeeper_->stop();
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
        socket_.close(ec);

        vm_.stop();
    }

    void ipt_session::logout() { cluster_bus_.pty_logout(dev_, vm_.get_tag()); }

    boost::asio::ip::tcp::endpoint ipt_session::get_remote_endpoint() const { return socket_.remote_endpoint(); }

    void ipt_session::start(std::chrono::seconds timeout) {
        do_read();
        gatekeeper_ = ctl_.create_channel_with_ref<gatekeeper>(logger_, this->shared_from_this(), cluster_bus_);
        BOOST_ASSERT(gatekeeper_->is_open());
        CYNG_LOG_TRACE(logger_, "start gatekeeper with a timeout of " << timeout.count() << " seconds");
        gatekeeper_->suspend(timeout, "timeout", cyng::make_tuple());
    }

    void ipt_session::do_read() {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    CYNG_LOG_DEBUG(
                        logger_, "[session] " << vm_.get_tag() << " received " << bytes_transferred << " bytes from ["
                                              << socket_.remote_endpoint() << "]");

                    //
                    //	let parse it
                    //
                    auto const data = parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
#ifdef _DEBUG_IPT
                    {
                        std::stringstream ss;
                        cyng::io::hex_dump<8> hd;
                        hd(ss, data.begin(), data.end());
                        CYNG_LOG_DEBUG(
                            logger_, "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes ip-t data:\n"
                                         << ss.str());
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
                }
            });
    }

    void ipt_session::do_write() {
        // if (is_stopped())	return;

        // Start an asynchronous operation to send a heartbeat message.
        boost::asio::async_write(
            socket_, boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            cyng::expose_dispatcher(vm_).wrap(std::bind(&ipt_session::handle_write, this, std::placeholders::_1)));
    }

    void ipt_session::handle_write(const boost::system::error_code &ec) {
        // if (is_stopped())	return;

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
            // else {

            //	// Wait 10 seconds before sending the next heartbeat.
            //	//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
            //	//heartbeat_timer_.async_wait(std::bind(&bus::do_write, this));
            //}
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
                auto const [name, pwd] = ipt::ctrl_req_login_public(std::move(body));
                CYNG_LOG_INFO(logger_, "[ipt] public login: " << name << ':' << pwd);
                cluster_bus_.pty_login(name, pwd, vm_.get_tag(), "sml", socket_.remote_endpoint());
            } else {
                CYNG_LOG_WARNING(logger_, "[session] login rejected - MALFUNCTION");
                ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
            }
            break;
        case ipt::code::CTRL_REQ_LOGIN_SCRAMBLED:
            if (cluster_bus_.is_connected()) {
                auto const [name, pwd, sk] = ipt::ctrl_req_login_scrambled(std::move(body));
                CYNG_LOG_INFO(logger_, "[ipt] scrambled login: " << name << ':' << pwd << ':' << ipt::to_string(sk));
                cluster_bus_.pty_login(name, pwd, vm_.get_tag(), "sml", socket_.remote_endpoint());
                parser_.set_sk(sk);
                serializer_.set_sk(sk);
            } else {
                CYNG_LOG_WARNING(logger_, "[session] login rejected - MALFUNCTION");
                ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
            }
            break;
        case ipt::code::APP_RES_SOFTWARE_VERSION:
            update_software_version(ipt::app_res_software_version(std::move(body)));
            break;
        case ipt::code::APP_RES_DEVICE_IDENTIFIER:
            update_device_identifier(ipt::app_res_device_identifier(std::move(body)));
            break;
        case ipt::code::CTRL_REQ_REGISTER_TARGET:
            if (cluster_bus_.is_connected()) {
                auto const [name, paket_size, window_size] = ipt::ctrl_req_register_target(std::move(body));
                register_target(name, paket_size, window_size, h.sequence_);
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
                auto const [target, account, msisdn, version, id, timeout] = ipt::tp_req_open_push_channel(std::move(body));
                open_push_channel(target, account, msisdn, version, id, timeout, h.sequence_);
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
#ifdef _DEBUG_IPT
                {
                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, body.begin(), body.end());
                    CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] <-- incoming push data:\n" << ss.str());
                }
#endif
                auto const [channel, source, status, block, data] = ipt::tp_req_pushdata_transfer(std::move(body));
                pushdata_transfer(channel, source, status, block, data, h.sequence_);
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

                //
                //	search for temporary connection establishment data
                //
                auto const pos = oce_map_.find(h.sequence_);
                if (pos != oce_map_.end()) {

                    auto const reader = cyng::make_reader(pos->second);
                    auto const caller_tag = cyng::value_cast(reader["caller-tag"].get(), boost::uuids::nil_uuid());
                    auto const caller_vm = cyng::value_cast(reader["caller-vm"].get(), boost::uuids::nil_uuid());
                    BOOST_ASSERT(!caller_tag.is_nil());
                    BOOST_ASSERT(!caller_vm.is_nil());

                    CYNG_LOG_TRACE(logger_, "[ipt] forward response to " << caller_tag << " on vm:" << caller_vm);

                    cluster_bus_.pty_res_open_connection(
                        ipt::tp_res_open_connection_policy::is_success(res), caller_vm, dev_, vm_.get_tag(),
                        std::move(pos->second));

                    //
                    //	cleanup oce map
                    //
                    oce_map_.erase(pos);
                } else {
                    CYNG_LOG_WARNING(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " without request");
                }
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
                auto const [res, channel, source, ps, ws, status, count] = ipt::tp_res_open_push_channel(std::move(body));
                CYNG_LOG_WARNING(
                    logger_, "[ipt] response open push channel " << channel << ':' << source << ": "
                                                                 << ipt::tp_res_open_push_channel_policy::get_response_name(res));
            }
            break;
        case ipt::code::UNKNOWN:
            CYNG_LOG_WARNING(logger_, "[ipt] unknown command " << ipt::ctrl_res_unknown_cmd(std::move(body)));
            break;
        default:
            CYNG_LOG_WARNING(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " dropped");
            ipt_send(serializer_.res_unknown_command(h.sequence_, h.command_));
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
        std::string name, std::string account, std::string msisdn, std::string version, std::string id, std::uint16_t timeout,
        ipt::sequence_t seq) {

        CYNG_LOG_INFO(logger_, "[ipt] open push channel: " << name);
        cluster_bus_.pty_open_channel(
            name, account, msisdn, version, id, std::chrono::seconds(timeout), dev_, vm_.get_tag(),
            cyng::param_map_factory("seq", seq));
    }

    void ipt_session::close_push_channel(std::uint32_t channel, ipt::sequence_t seq) {

        CYNG_LOG_INFO(logger_, "[ipt] close push channel: " << channel);
        cluster_bus_.pty_close_channel(channel, dev_, vm_.get_tag(), cyng::param_map_factory("seq", seq));
    }

    void ipt_session::pushdata_transfer(
        std::uint32_t channel, std::uint32_t source, std::uint8_t status, std::uint8_t block, cyng::buffer_t data,
        ipt::sequence_t seq) {

        auto const size = data.size();

        CYNG_LOG_INFO(logger_, "[ipt] pushdata transfer " << channel << ':' << source << ", " << size << " bytes");
        cluster_bus_.pty_push_data(
            channel, source, std::move(data), dev_, vm_.get_tag(),
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
        //
        cyng::param_map_t const data = cyng::param_map_factory()("id", str);
        cluster_bus_.req_db_update("device", cyng::key_generator(dev_), data);
    }

    void ipt_session::ipt_stream(cyng::buffer_t &&data) {
        CYNG_LOG_TRACE(logger_, "ipt stream " << data.size() << " byte");
#ifdef _DEBUG_IPT
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] " << data.size() << " stream bytes:\n" << ss.str());
        }
#endif
        if (cluster_bus_.is_connected()) {
            cluster_bus_.pty_transfer_data(dev_, vm_.get_tag(), std::move(data));
        }
    }

    void ipt_session::ipt_send(cyng::buffer_t &&data) {
        cyng::exec(vm_, [this, data]() {
            bool const b = buffer_write_.empty();
            buffer_write_.push_back(data);
            if (b)
                do_write();
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
            ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::SUCCESS, 0, ""));

            query();

        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
            ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::UNKNOWN_ACCOUNT, 0, ""));

            //
            //  gatekeeper will close this session
            //
        }
    }

    void ipt_session::pty_res_register(bool success, std::uint32_t channel, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " register target ok: " << token);

            ipt_send(serializer_.res_register_push_target(seq, ipt::ctrl_res_register_target_policy::OK, channel));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " register target failed: " << token);
            ipt_send(serializer_.res_register_push_target(seq, ipt::ctrl_res_register_target_policy::REJECTED, channel));
        }
    }

    void ipt_session::pty_res_open_channel(
        bool success, std::uint32_t channel, std::uint32_t source, std::uint16_t packet_size, std::uint8_t window_size,
        std::uint8_t status, std::uint32_t count, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(
                logger_, "[pty] " << vm_.get_tag() << " open push channel " << channel << ':' << source << " ok: " << token);

            ipt_send(serializer_.res_open_push_channel(
                seq, ipt::tp_res_open_push_channel_policy::SUCCESS, channel, source, packet_size, window_size, status, count));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " open push channel failed: " << token);
            ipt_send(serializer_.res_open_push_channel(seq, ipt::tp_res_open_push_channel_policy::UNREACHABLE, 0, 0, 0, 1, 0, 0));
        }
    }

    void ipt_session::pty_req_push_data(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data) {

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " push data " << data.size() << " bytes");
#ifdef _DEBUG_IPT
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] --> outgoing push data:\n" << ss.str());
        }
#endif
        std::uint8_t status = 0;
        std::uint8_t block = 0;
        ipt_send(serializer_.req_transfer_push_data(channel, source, status, block, std::move(data)));
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
                    logger_, "[pty] " << vm_.get_tag() << " push data " << channel << ':' << source
                                      << " invalid push channel status: " << +status);
            }

            ipt_send(serializer_.res_transfer_push_data(
                seq, ipt::tp_res_pushdata_transfer_policy::SUCCESS, channel, source,
                status | ipt::tp_res_pushdata_transfer_policy::ACK, block));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " push data " << channel << " failed: " << token);
            ipt_send(serializer_.res_transfer_push_data(
                seq, ipt::tp_res_pushdata_transfer_policy::UNREACHABLE, channel, source, status, block));
        }
    }

    void ipt_session::pty_res_close_channel(bool success, std::uint32_t channel, std::size_t count, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        BOOST_ASSERT(seq != 0);
        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " close push channel " << channel << " ok: " << token);
            ipt_send(serializer_.res_close_push_channel(seq, ipt::tp_res_close_push_channel_policy::SUCCESS, channel));
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " close push channel " << channel << " failed: " << token);
            ipt_send(serializer_.res_close_push_channel(seq, ipt::tp_res_close_push_channel_policy::BROKEN, channel));
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

        ipt_send(serializer_.res_open_connection(
            seq, success ? ipt::tp_res_open_connection_policy::DIALUP_SUCCESS : ipt::tp_res_open_connection_policy::DIALUP_FAILED));
    }

    void ipt_session::pty_res_close_connection(bool success, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);
        auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed " << token);

        BOOST_ASSERT(seq != 0);
        ipt_send(serializer_.res_close_connection(
            seq, success ? ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED
                         : ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED));
    }

    void ipt_session::pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token) {

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " req open connection " << msisdn);
        // std::pair<cyng::buffer_t, sequence_t>
        auto r = serializer_.req_open_connection(msisdn);
        ipt_send(std::move(r.first));
        oce_map_.emplace(r.second, token);
    }

    void ipt_session::pty_transfer_data(cyng::buffer_t data) {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " transfer " << data.size() << " byte");
#ifdef _DEBUG_IPT
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] emit " << data.size() << " bytes:\n" << ss.str());
        }
#endif
        ipt_send(serializer_.write(data));
        // ipt_send(serializer_.scramble(std::move(data)));
    }

    void ipt_session::pty_req_close_connection() {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " close connection");
        ipt_send(serializer_.req_close_connection());
    }

    void ipt_session::query() {

        if (ipt::test_bit(query_, ipt::query::PROTOCOL_VERSION)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::PROTOCOL_VERSION)));
            ipt_send(serializer_.req_protocol_version());
        }
        if (ipt::test_bit(query_, ipt::query::FIRMWARE_VERSION)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::FIRMWARE_VERSION)));
            ipt_send(serializer_.req_software_version());
        }
        if (ipt::test_bit(query_, ipt::query::DEVICE_IDENTIFIER)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_IDENTIFIER)));
            ipt_send(serializer_.req_device_id());
        }
        if (ipt::test_bit(query_, ipt::query::NETWORK_STATUS)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::NETWORK_STATUS)));
            ipt_send(serializer_.req_network_status());
        }
        if (ipt::test_bit(query_, ipt::query::IP_STATISTIC)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::IP_STATISTIC)));
            ipt_send(serializer_.req_ip_statistics());
        }
        if (ipt::test_bit(query_, ipt::query::DEVICE_AUTHENTIFICATION)) {
            CYNG_LOG_TRACE(
                logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_AUTHENTIFICATION)));
            ipt_send(serializer_.req_device_auth());
        }
        if (ipt::test_bit(query_, ipt::query::DEVICE_TIME)) {
            CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_TIME)));
            ipt_send(serializer_.req_device_time());
        }
    }

    auto ipt_session::get_vm_func_pty_res_login(ipt_session *ptr) -> std::function<void(bool success, boost::uuids::uuid)> {
        return std::bind(&ipt_session::pty_res_login, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    auto ipt_session::get_vm_func_pty_res_register(ipt_session *ptr)
        -> std::function<void(bool success, std::uint32_t, cyng::param_map_t)> {
        return std::bind(&ipt_session::pty_res_register, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    auto ipt_session::get_vm_func_pty_res_open_channel(ipt_session *ptr) -> std::function<void(
        bool success, std::uint32_t, std::uint32_t, std::uint16_t, std::uint8_t, std::uint8_t, std::uint32_t, cyng::param_map_t)> {
        return std::bind(
            &ipt_session::pty_res_open_channel, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
            std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8);
    }

    auto ipt_session::get_vm_func_pty_req_push_data(ipt_session *ptr)
        -> std::function<void(std::uint32_t, std::uint32_t, cyng::buffer_t)> {

        return std::bind(&ipt_session::pty_req_push_data, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    auto ipt_session::get_vm_func_pty_res_push_data(ipt_session *ptr)
        -> std::function<void(bool, std::uint32_t, std::uint32_t, cyng::param_map_t)> {

        return std::bind(
            &ipt_session::pty_res_push_data, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
            std::placeholders::_4);
    }

    auto ipt_session::get_vm_func_pty_res_close_channel(ipt_session *ptr)
        -> std::function<void(bool success, std::uint32_t channel, std::size_t count, cyng::param_map_t)> {
        return std::bind(
            &ipt_session::pty_res_close_channel, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
            std::placeholders::_4);
    }

    auto ipt_session::get_vm_func_pty_res_open_connection(ipt_session *ptr)
        -> std::function<void(bool success, cyng::param_map_t)> {
        return std::bind(&ipt_session::pty_res_open_connection, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    auto ipt_session::get_vm_func_pty_transfer_data(ipt_session *ptr) -> std::function<void(cyng::buffer_t)> {
        return std::bind(&ipt_session::pty_transfer_data, ptr, std::placeholders::_1);
    }

    auto ipt_session::get_vm_func_pty_res_close_connection(ipt_session *ptr)
        -> std::function<void(bool success, cyng::param_map_t)> {
        return std::bind(&ipt_session::pty_res_close_connection, ptr, std::placeholders::_1, std::placeholders::_2);
    }

    auto ipt_session::get_vm_func_pty_req_open_connection(ipt_session *ptr)
        -> std::function<void(std::string, bool, cyng::param_map_t)> {
        return std::bind(
            &ipt_session::pty_req_open_connection, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    auto ipt_session::get_vm_func_pty_req_close_connection(ipt_session *ptr) -> std::function<void()> {
        return std::bind(&ipt_session::pty_req_close_connection, ptr);
    }

} // namespace smf
