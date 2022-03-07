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
#include <cyng/parse/string.h>
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
        , auto_answer_(auto_answer)
        , logger_(logger)
        , cluster_bus_(cluster_bus)
        , buffer_()
        , rx_{0}
        , sx_{0}
        , px_{0}
        , buffer_write_()
        , parser_(
              [&, this](std::string const &cmd, std::string const &data) {
                  if (boost::algorithm::equals(cmd, "AT+")) {
                      //    AT+name?password
                      auto const vec = cyng::split(data, "?");
                      if (vec.size() == 2) {
                          CYNG_LOG_INFO(logger_, "login: " << vec.at(0) << ':' << vec.at(1));
                          cluster_bus_.pty_login(vec.at(0), vec.at(1), vm_.get_tag(), "modem", socket_.remote_endpoint());
                      } else {
                          CYNG_LOG_WARNING(logger_, "incomplete login data: " << data);
                      }
                  } else if (boost::algorithm::equals(cmd, "ATD")) {
                      CYNG_LOG_INFO(logger_, "dial: " << data);
                      cluster_bus_.pty_open_connection(
                          data, dev_, vm_.get_tag(), cyng::param_map_factory("auto-answer", auto_answer));
                  } else if (boost::algorithm::equals(cmd, "+++")) {
                      //    back to command mode
                      CYNG_LOG_INFO(logger_, "back to command mode");
                      // parser_.set_cmd_mode();
                      print(serializer_.ok());
                  } else if (boost::algorithm::equals(cmd, "ATH")) {
                      //    hang up
                      CYNG_LOG_INFO(logger_, "hang up");
                      cluster_bus_.pty_close_connection(
                          dev_, vm_.get_tag(), cyng::param_map_factory("tp", std::chrono::system_clock::now()));
                      //    wait for close response
                      // print(serializer_.ok());
                  } else if (boost::algorithm::equals(cmd, "ATA")) {
                      //    call answer
                      CYNG_LOG_INFO(logger_, "call answer");
                      // parser_.set_stream_mode();
                      // cluster_bus_.pty_res_open_connection(); //  insert stored token
                  } else if (boost::algorithm::equals(cmd, "ATI")) {
                      //    print info
                      try {
                          const auto info = std::stoul(data);
                          //                p.cb_(cyng::generate_invoke("modem.req.info", cyng::code::IDENT,
                          //                static_cast<std::uint32_t>(info)));
                          print(serializer_.ok());
                      } catch (std::exception const &ex) {
                          boost::ignore_unused(ex);
                          //                p.cb_(cyng::generate_invoke("modem.req.info", cyng::code::IDENT,
                          //                static_cast<std::uint32_t>(0u)));
                          print(serializer_.error());
                      }
                  } else {
                      CYNG_LOG_WARNING(logger_, "unknown AT command" << cmd << ": " << data);
                  }
              },
              [&, this](cyng::buffer_t &&data) {
                  //    tarnsfer data over open connection
                  cluster_bus_.pty_transfer_data(this->dev_, this->vm_.get_tag(), std::move(data));
              },
              guard)
        , serializer_()
        , vm_()
        , dev_(boost::uuids::nil_uuid())
        , gatekeeper_() {
        vm_ = fabric.make_proxy(
            cluster_bus_.get_tag(),
            cyng::make_description(
                "pty.res.login",
                cyng::vm_adaptor<modem_session, void, bool, boost::uuids::uuid>(this, &modem_session::pty_res_login)),
            // cyng::make_description("pty.res.register", get_vm_func_pty_res_register(this)),
            // cyng::make_description("pty.res.open.channel", get_vm_func_pty_res_open_channel(this)),
            // cyng::make_description("pty.req.push.data", get_vm_func_pty_req_push_data(this)),
            // cyng::make_description("pty.push.data.res", get_vm_func_pty_res_push_data(this)),
            // cyng::make_description("pty.res.close.channel", get_vm_func_pty_res_close_channel(this)),
            cyng::make_description(
                "pty.res.open.connection",
                cyng::vm_adaptor<modem_session, void, bool, cyng::param_map_t>(this, &modem_session::pty_res_open_connection)),

            cyng::make_description(
                "pty.transfer.data",
                cyng::vm_adaptor<modem_session, void, cyng::buffer_t>(this, &modem_session::pty_transfer_data)),

            cyng::make_description(
                "pty.res.close.connection",
                cyng::vm_adaptor<modem_session, void, bool, cyng::param_map_t>(this, &modem_session::pty_res_close_connection)),

            cyng::make_description(
                "pty.req.open.connection",
                cyng::vm_adaptor<modem_session, void, std::string, bool, cyng::param_map_t>(
                    this, &modem_session::pty_req_open_connection)),

            cyng::make_description(
                "pty.req.close.connection", cyng::vm_adaptor<modem_session, void>(this, &modem_session::pty_req_close_connection)),

            cyng::make_description("pty.req.stop", cyng::vm_adaptor<modem_session, void>(this, &modem_session::pty_stop)),

            cyng::make_description(
                "cfg.req.backup",
                cyng::vm_adaptor<
                    modem_session,
                    void,
                    std::string,
                    std::string,
                    boost::uuids::uuid,
                    cyng::buffer_t,
                    std::string, //  firware version
                    std::chrono::system_clock::time_point>(this, &modem_session::cfg_backup)));

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
        if (vm_.stop()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
            socket_.close(ec);
        }
    }

    void modem_session::logout() { cluster_bus_.pty_logout(dev_, vm_.get_tag()); }

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

                // if (gatekeeper_->is_open()) {
                //     gatekeeper_->stop();
                // }
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

    void modem_session::print(cyng::buffer_t &&data) {
        cyng::exec(vm_, [this, data]() {
            bool const b = buffer_write_.empty();
            buffer_write_.push_back(std::move(data));
            if (b)
                do_write();
        });
    }

    void modem_session::pty_res_login(bool success, boost::uuids::uuid dev) {
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
            print(serializer_.ok());
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
            print(serializer_.error());
        }
    }

    void modem_session::pty_res_open_connection(bool success, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);
        auto const auto_answer = cyng::value_cast(reader["auto-answer"].get(), true);

        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " dialup ok: " << token);
            print(serializer_.connect());
            parser_.set_stream_mode();

        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " dialup failed: " << token);
            print(serializer_.no_answer());
        }
    }

    void modem_session::pty_res_close_connection(bool success, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed (response)" << token);

        print(success ? serializer_.no_carrier() : serializer_.error());

        // ipt_send(std::bind(
        //     &ipt::serializer::res_close_connection,
        //     &serializer_,
        //     seq,
        //     success ? ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED
        //     : ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED));
    }

    void modem_session::pty_transfer_data(cyng::buffer_t data) {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " transfer " << data.size() << " byte");
#ifdef _DEBUG_MODEM
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] emit " << data.size() << " bytes:\n" << dmp);
        }
#endif
        //  print binary data
        print(std::move(data));
    }

    void modem_session::pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token) {

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " req open connection " << msisdn);
        if (auto_answer_) {
            //  auto response
            cluster_bus_.pty_res_open_connection(true, dev_, vm_.get_tag(), std::move(token));
            parser_.set_stream_mode();
        } else {
            print(serializer_.ring());
            //  store "token" and wait for ATA
        }
    }

    void modem_session::pty_req_close_connection() {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed (request)");
        print(serializer_.no_carrier());
        // ipt_send(std::bind(&ipt::serializer::req_close_connection, &serializer_));
    }

    void modem_session::pty_stop() { stop(); }

    void modem_session::cfg_backup(
        std::string name,
        std::string pwd,
        boost::uuids::uuid,
        cyng::buffer_t id,
        std::string, //  firware version
        std::chrono::system_clock::time_point tp) {
        CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " backup: " << name << ':' << pwd << '@' << id);
    }

} // namespace smf
