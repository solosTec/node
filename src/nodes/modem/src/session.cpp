/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <session.h>
#include <tasks/gatekeeper.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>

#ifdef _DEBUG_MODEM
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    modem_session::modem_session(
        boost::asio::ip::tcp::socket socket,
        bus &cluster_bus,
        cyng::mesh &fabric,
        bool auto_answer,
        std::chrono::milliseconds guard,
        cyng::logger logger)
        : ctl_(fabric.get_ctl())
        , socket_(std::move(socket))
        , logger_(logger)
        , cluster_bus_(cluster_bus)
        , buffer_()
        , rx_{0}
        , sx_{0}
        , px_{0}
        , buffer_write_()
        , parser_(
              [&, this](std::pair<std::string, std::string> const &cmd) {
                  CYNG_LOG_DEBUG(logger_, "cmd: " << cmd.first << ": " << cmd.second);
              },
              [](cyng::buffer_t &&data) {},
              guard)
        //, serializer_(sk)
        , vm_()
        //, dev_(boost::uuids::nil_uuid())
        , gatekeeper_() {
        vm_ = fabric.make_proxy(cluster_bus_.get_tag()
                                // cyng::make_description("pty.res.login", get_vm_func_pty_res_login(this)),
                                // cyng::make_description("pty.res.register", get_vm_func_pty_res_register(this)),
                                // cyng::make_description("pty.res.open.channel", get_vm_func_pty_res_open_channel(this)),
                                // cyng::make_description("pty.push.data.req", get_vm_func_pty_req_push_data(this)),
                                // cyng::make_description("pty.push.data.res", get_vm_func_pty_res_push_data(this)),
                                // cyng::make_description("pty.res.close.channel", get_vm_func_pty_res_close_channel(this)),
                                // cyng::make_description("pty.res.open.connection", get_vm_func_pty_res_open_connection(this)),
                                // cyng::make_description("pty.transfer.data", get_vm_func_pty_transfer_data(this)),
                                // cyng::make_description("pty.res.close.connection", get_vm_func_pty_res_close_connection(this)),
                                // cyng::make_description("pty.req.open.connection", get_vm_func_pty_req_open_connection(this)),
                                // cyng::make_description("pty.req.close.connection", get_vm_func_pty_req_close_connection(this))
        );

        CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << socket_.remote_endpoint() << " created");
    }

    modem_session::~modem_session() {
        gatekeeper_->stop();
#ifdef _DEBUG_IPT
        std::cout << "session(~)" << std::endl;
#endif
    }

    void modem_session::stop() {
        //	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
        socket_.close(ec);

        vm_.stop();
    }

    void modem_session::logout() { // cluster_bus_.pty_logout(dev_, vm_.get_tag());
    }

    boost::asio::ip::tcp::endpoint modem_session::get_remote_endpoint() const { return socket_.remote_endpoint(); }

    void modem_session::start(std::chrono::seconds timeout) {
        do_read();
        gatekeeper_ = ctl_.create_channel_with_ref<gatekeeper>(logger_, this->shared_from_this(), cluster_bus_);
        BOOST_ASSERT(gatekeeper_->is_open());
        CYNG_LOG_TRACE(logger_, "start gatekeeper with a timeout of " << timeout.count() << " seconds");
        gatekeeper_->suspend(timeout, "timeout");
    }

    void modem_session::do_read() {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    CYNG_LOG_DEBUG(
                        logger_,
                        "[session] " << vm_.get_tag() << " received " << bytes_transferred << " bytes from ["
                                     << socket_.remote_endpoint() << "]");

                    // if (bytes_transferred == 45) {
                    //    int i = 0; //  start debugging here
                    //               //  [0000]  f9 0c e2 29 87 b1 2a 3b  4a 4a 44 74 6a be 03 e1  ...)..*; JJDtj...
                    //               //  garble data from wMBus broker to oen a second channel when scrambling is active
                    //}

                    if (gatekeeper_->is_open()) {
                        gatekeeper_->stop();
                    }
#ifdef _DEBUG_MODEM
                    {
                        std::stringstream ss;
                        cyng::io::hex_dump<8> hd;
                        hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
                        auto const dmp = ss.str();

                        CYNG_LOG_DEBUG(
                            logger_,
                            "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n"
                                << dmp);
                    }
#endif
                    //
                    //	let parse it
                    //
                    // auto const data =
                    parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
#ifdef __DEBUG_MODEM
                    {
                        std::stringstream ss;
                        cyng::io::hex_dump<8> hd;
                        hd(ss, data.begin(), data.end());
                        auto const dmp = ss.str();
                        CYNG_LOG_DEBUG(
                            logger_,
                            "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes (unscrambled) ip-t data:\n"
                                << dmp);
                    }
#endif

                    //
                    //	update rx
                    //
                    rx_ += static_cast<std::uint64_t>(bytes_transferred);
                    // if (!dev_.is_nil() && cluster_bus_.is_connected()) {

                    //    cluster_bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("rx", rx_));
                    //}

                    //
                    //	continue reading
                    //
                    do_read();
                } else {
                    CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " read: " << ec.message());
                }
            });
    }

    void modem_session::do_write() {
        // if (is_stopped())	return;

        // Start an asynchronous operation to send a heartbeat message.
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            cyng::expose_dispatcher(vm_).wrap(std::bind(&modem_session::handle_write, this, std::placeholders::_1)));
    }

    void modem_session::handle_write(const boost::system::error_code &ec) {
        // if (is_stopped())	return;

        if (!ec) {

            //
            //	update sx
            //
            sx_ += static_cast<std::uint64_t>(buffer_write_.front().size());
            // if (!dev_.is_nil() && cluster_bus_.is_connected()) {

            //    cluster_bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("sx", sx_));
            //}

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

} // namespace smf
