#include <session.h>
#include <tasks/gatekeeper.h>

#include <smf/config/protocols.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/string.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>

#ifdef _DEBUG_IMEGA
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    imega_session::imega_session(
        boost::asio::ip::tcp::socket socket,
        bus &cluster_bus,
        cyng::mesh &fabric,
        cyng::logger logger,
        imega::policy policy,
        std::string const &pwd)
        : ctl_(fabric.get_ctl())
        , socket_(std::move(socket))
        , logger_(logger)
        , cluster_bus_(cluster_bus)
        , buffer_()
        , rx_{0}
        , sx_{0}
        , buffer_write_()
        , parser_(
              [=, this](std::uint8_t protocol, cyng::version v, std::string telnb, std::string mname) {
                  //  login
                  switch (policy) {
                  case imega::policy::MNAME:
                      CYNG_LOG_INFO(
                          logger_,
                          "[pty] login MNAME " << vm_.get_tag() << ", " << mname << ':' << telnb << '@'
                                               << socket_.remote_endpoint());
                      cluster_bus_.pty_login(
                          mname, mname, vm_.get_tag(), config::get_name(config::protocol::DLMS), socket_.remote_endpoint());
                      break;
                  case imega::policy::TELNB:
                      CYNG_LOG_INFO(
                          logger_,
                          "[pty] login TELNB " << vm_.get_tag() << ", " << mname << ':' << telnb << '@'
                                               << socket_.remote_endpoint());
                      cluster_bus_.pty_login(
                          telnb, mname, vm_.get_tag(), config::get_name(config::protocol::DLMS), socket_.remote_endpoint());
                      break;
                  default:
                      //    global
                      CYNG_LOG_INFO(
                          logger_,
                          "[pty] login GLOBAL " << vm_.get_tag() << ", " << mname << ':' << telnb << ':' << pwd << '@'
                                                << socket_.remote_endpoint());
                      cluster_bus_.pty_login(
                          mname, pwd, vm_.get_tag(), config::get_name(config::protocol::DLMS), socket_.remote_endpoint());
                      break;
                  }
              },
              [&, this]() {
                  //  watchdog
                  CYNG_LOG_TRACE(logger_, "[pty] watchdog " << vm_.get_tag() << " from " << socket_.remote_endpoint());
              },
              [&, this](cyng::buffer_t &&data, bool ok) {
                  if (ok) {
                      //    transfer data over open connection
                      CYNG_LOG_TRACE(
                          logger_, "[pty] data " << vm_.get_tag() << " from " << socket_.remote_endpoint() << ": " << data);
                      cluster_bus_.pty_transfer_data(this->dev_, this->vm_.get_tag(), std::move(data));
                  } else {
                      CYNG_LOG_WARNING(
                          logger_, "[pty] data " << vm_.get_tag() << " from " << socket_.remote_endpoint() << ": " << data);
                  }
              })
        , serializer_()
        , vm_()
        , dev_(boost::uuids::nil_uuid())
        , gatekeeper_() {
        vm_ = fabric.make_proxy(
            cluster_bus_.get_tag(),
            //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &cluster_bus_, std::placeholders::_1, std::placeholders::_2),
            cyng::make_description(
                "pty.res.login",
                cyng::vm_adaptor<imega_session, void, bool, boost::uuids::uuid>(this, &imega_session::pty_res_login)),
            // cyng::make_description("pty.res.register", get_vm_func_pty_res_register(this)),
            // cyng::make_description("pty.res.open.channel", get_vm_func_pty_res_open_channel(this)),
            // cyng::make_description("pty.req.push.data", get_vm_func_pty_req_push_data(this)),
            // cyng::make_description("pty.push.data.res", get_vm_func_pty_res_push_data(this)),
            // cyng::make_description("pty.res.close.channel", get_vm_func_pty_res_close_channel(this)),
            cyng::make_description(
                "pty.res.open.connection",
                cyng::vm_adaptor<imega_session, void, bool, cyng::param_map_t>(this, &imega_session::pty_res_open_connection)),

            cyng::make_description(
                "pty.transfer.data",
                cyng::vm_adaptor<imega_session, void, cyng::buffer_t>(this, &imega_session::pty_transfer_data)),

            cyng::make_description(
                "pty.res.close.connection",
                cyng::vm_adaptor<imega_session, void, bool, cyng::param_map_t>(this, &imega_session::pty_res_close_connection)),

            cyng::make_description(
                "pty.req.open.connection",
                cyng::vm_adaptor<imega_session, void, std::string, bool, cyng::param_map_t>(
                    this, &imega_session::pty_req_open_connection)),

            cyng::make_description(
                "pty.req.close.connection", cyng::vm_adaptor<imega_session, void>(this, &imega_session::pty_req_close_connection)),

            cyng::make_description("pty.req.stop", cyng::vm_adaptor<imega_session, void>(this, &imega_session::pty_stop)),

            cyng::make_description(
                "cfg.req.backup",
                cyng::vm_adaptor<
                    imega_session,
                    void,
                    std::string,
                    std::string,
                    boost::uuids::uuid,
                    cyng::buffer_t,
                    std::string, //  firware version
                    std::chrono::system_clock::time_point>(this, &imega_session::cfg_backup)));

        CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << socket_.remote_endpoint() << " created");
    }

    imega_session::~imega_session() {
        gatekeeper_->stop();
#ifdef _DEBUG_IPT
        std::cout << "session(~)" << std::endl;
#endif
    }

    void imega_session::stop() {
        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
        if (vm_.stop()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
            socket_.close(ec);
        }
    }

    void imega_session::logout() { cluster_bus_.pty_logout(dev_, vm_.get_tag()); }

    boost::asio::ip::tcp::endpoint imega_session::get_remote_endpoint() const { return socket_.remote_endpoint(); }

    void imega_session::start(std::chrono::seconds timeout) {
        do_read();
        gatekeeper_ = ctl_.create_channel_with_ref<gatekeeper>(logger_, this->shared_from_this(), cluster_bus_).first;
        BOOST_ASSERT(gatekeeper_->is_open());
        CYNG_LOG_TRACE(logger_, "start gatekeeper with a timeout of " << timeout.count() << " seconds");

        gatekeeper_->suspend(
            timeout,
            "timeout", //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &cluster_bus_, std::placeholders::_1, std::placeholders::_2));
    }

    void imega_session::do_read() {
        auto self = shared_from_this();

        socket_.async_read_some(
            boost::asio::buffer(buffer_.data(), buffer_.size()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    CYNG_LOG_DEBUG(
                        logger_,
                        "[session] " << vm_.get_tag() << " received " << bytes_transferred << " bytes from ["
                                     << socket_.remote_endpoint() << "]");
#ifdef _DEBUG_IMEGA
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
#ifdef __DEBUG_IMEGA
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

    void imega_session::do_write() {
        // if (is_stopped())	return;

        // Start an asynchronous operation to send a heartbeat message.
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            cyng::expose_dispatcher(vm_).wrap(std::bind(&imega_session::handle_write, this, std::placeholders::_1)));
    }

    void imega_session::handle_write(const boost::system::error_code &ec) {
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

    void imega_session::pty_res_login(bool success, boost::uuids::uuid dev) {
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
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
        }
    }

    void imega_session::pty_res_open_connection(bool success, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);

        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " dialup ok: " << token);

        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " dialup failed: " << token);
        }
    }

    void imega_session::pty_res_close_connection(bool success, cyng::param_map_t token) {

        // auto const reader = cyng::make_reader(token);
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed (response)" << token);
    }

    void imega_session::pty_transfer_data(cyng::buffer_t data) {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " transfer " << data.size() << " bytes to " << dev_);
#ifdef _DEBUG_IMEGA
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] emit " << data.size() << " bytes:\n" << dmp);
        }
#endif
        //  send to device
        imega_send([=, this]() mutable -> cyng::buffer_t { return serializer_.raw_data(std::move(data)); });
    }

    void imega_session::imega_send(std::function<cyng::buffer_t()> f) {
        cyng::exec(vm_, [this, f]() {
            bool const b = buffer_write_.empty();
            buffer_write_.push_back(f());
            if (b) {
                do_write();
            }
        });
    }

    void imega_session::pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token) {

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " req open connection " << msisdn);
        //  auto response
        cluster_bus_.pty_res_open_connection(true, dev_, vm_.get_tag(), std::move(token));
    }

    void imega_session::pty_req_close_connection() {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed (request)");
        // always successful
    }

    void imega_session::pty_stop() { stop(); }

    void imega_session::cfg_backup(
        std::string name,
        std::string pwd,
        boost::uuids::uuid,
        cyng::buffer_t id,
        std::string, //  firware version
        std::chrono::system_clock::time_point tp) {
        CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " backup: " << name << ':' << pwd << '@' << id);
    }

} // namespace smf
