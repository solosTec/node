#include <session.h>

#include <smf/config/protocols.h>

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
        cyng::logger logger,
        bool auto_answer,
        std::chrono::milliseconds guard)
        : base_t(
              std::move(socket),
              cluster_bus,
              fabric,
              logger,
              modem::parser(
                  [&, this](std::string const &cmd, std::string const &data) {
                      if (boost::algorithm::equals(cmd, "AT+")) {
                          //    AT+name?password
                          auto const vec = cyng::split(data, "?");
                          if (vec.size() == 2) {
                              CYNG_LOG_INFO(logger_, "[" << parser_.get_mode() << "] login: " << vec.at(0) << ':' << vec.at(1));
                              bus_.pty_login(
                                  vec.at(0),
                                  vec.at(1),
                                  vm_.get_tag(),
                                  config::get_name(config::protocol::HAYESAT),
                                  get_remote_endpoint());
                          } else {
                              CYNG_LOG_WARNING(logger_, "[" << parser_.get_mode() << "] incomplete login data: " << data);
                          }
                      } else if (boost::algorithm::equals(cmd, "ATD")) {
                          CYNG_LOG_INFO(logger_, "[" << parser_.get_mode() << "] dial: " << data);
                          bus_.pty_open_connection(data, dev_, vm_.get_tag(), cyng::param_map_factory("auto-answer", auto_answer));
                      } else if (boost::algorithm::equals(cmd, "+++")) {
                          //    back to command mode
                          CYNG_LOG_INFO(logger_, "[" << parser_.get_mode() << "] back to command mode");
                          // parser_.set_cmd_mode();
                          send([this]() -> cyng::buffer_t { return serializer_.ok(); });
                      } else if (boost::algorithm::equals(cmd, "ATH")) {
                          //    hang up
                          CYNG_LOG_INFO(logger_, "[" << parser_.get_mode() << "] hang up");
                          bus_.pty_close_connection(
                              dev_, vm_.get_tag(), cyng::param_map_factory("tp", std::chrono::system_clock::now()));
                          //    wait for close response
                          // send(serializer_.ok());
                      } else if (boost::algorithm::equals(cmd, "ATA")) {
                          //    call answer
                          CYNG_LOG_INFO(logger_, "[" << parser_.get_mode() << "] call answer");
                          // parser_.set_stream_mode();
                          // bus_.pty_res_open_connection(); //  insert stored token
                      } else if (boost::algorithm::equals(cmd, "ATI")) {
                          //    print info
                          try {
                              const auto info = std::stoul(data);
                              CYNG_LOG_INFO(logger_, "[" << parser_.get_mode() << "] AT#" << info);
                              switch (info) {
                              case 1:
                                  //    local IP address
                                  send([this]() -> cyng::buffer_t { return serializer_.ep(get_local_endpoint()); });
                                  break;
                              case 2:
                                  //    remote IP address
                                  send([this]() -> cyng::buffer_t { return serializer_.ep(get_remote_endpoint()); });
                                  break;
                              default:
                                  // unknown info
                                  send([this]() -> cyng::buffer_t { return serializer_.error(); });
                                  break;
                              }
                          } catch (std::exception const &ex) {
                              boost::ignore_unused(ex);
                              send([this]() -> cyng::buffer_t { return serializer_.error(); });
                          }
                      } else {
                          CYNG_LOG_WARNING(logger_, "[" << parser_.get_mode() << "] unknown AT command" << cmd << ": " << data);
                      }
                  },
                  [&, this](cyng::buffer_t &&data) {
                      //    transfer data over open connection
                      bus_.pty_transfer_data(this->dev_, this->vm_.get_tag(), std::move(data));
                  },
                  guard),
              modem::serializer{})
        , auto_answer_(auto_answer) {

        //
        //  init VM
        //
        vm_ = fabric.make_proxy(
            bus_.get_tag(),
            //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
            cyng::make_description(
                "pty.res.login",
                cyng::vm_adaptor<modem_session, void, bool, boost::uuids::uuid>(this, &modem_session::pty_res_login)),
            // "pty.res.register" is not implemented
            // "pty.res.open.channel" is not implemented
            // "pty.req.push.data" is not implemented
            // "pty.push.data.res" is not implemented
            // "pty.res.close.channel" is not implemented
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

        CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << get_remote_endpoint() << " created");
    }

    modem_session::~modem_session() {
#ifdef _DEBUG_MODEM
        std::cout << "session(~)" << std::endl;
#endif
    }

    void modem_session::stop() {
        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
        if (vm_.stop()) {
            close_socket();
        }
    }

    void modem_session::logout() { bus_.pty_logout(dev_, vm_.get_tag()); }

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
            send([this]() -> cyng::buffer_t { return serializer_.ok(); });
        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
            send([this]() -> cyng::buffer_t { return serializer_.error(); });
        }
    }

    void modem_session::pty_res_open_connection(bool success, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);
        auto const auto_answer = cyng::value_cast(reader["auto-answer"].get(), true);

        if (success) {
            CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " dialup ok: " << token);
            send([this]() -> cyng::buffer_t { return serializer_.connect(); });
            parser_.set_stream_mode();

        } else {
            CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " dialup failed: " << token);
            send([this]() -> cyng::buffer_t { return serializer_.no_answer(); });
        }
    }

    void modem_session::pty_res_close_connection(bool success, cyng::param_map_t token) {

        auto const reader = cyng::make_reader(token);

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed (response)" << token);

        send([this, success]() -> cyng::buffer_t { return success ? serializer_.no_carrier() : serializer_.error(); });
    }

    void modem_session::pty_transfer_data(cyng::buffer_t data) {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " transfer " << data.size() << " byte");
#ifdef _DEBUG_MODEM
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[" << get_remote_endpoint() << "] emit " << data.size() << " bytes:\n" << dmp);
        }
#endif
        //  print binary data
        // send(std::move(data));
        send([data]() -> cyng::buffer_t { return data; });
    }

    void modem_session::pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token) {

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " req open connection " << msisdn);
        if (auto_answer_) {
            //  auto response
            bus_.pty_res_open_connection(true, dev_, vm_.get_tag(), std::move(token));
            parser_.set_stream_mode();
        } else {
            send([this]() -> cyng::buffer_t { return serializer_.ring(); });
            //  store "token" and wait for ATA
        }
    }

    void modem_session::pty_req_close_connection() {
        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " connection closed (request)");
        send([this]() -> cyng::buffer_t { return serializer_.no_carrier(); });
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
