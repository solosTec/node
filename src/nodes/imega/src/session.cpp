#include <session.h>

#include <smf/config/protocols.h>

#include <cyng/io/hex_dump.hpp>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/string.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <iostream>
#include <sstream>

namespace smf {

    imega_session::imega_session(
        boost::asio::ip::tcp::socket socket,
        bus &cluster_bus,
        cyng::mesh &fabric,
        cyng::logger logger,
        imega::policy policy,
        std::string const &pwd)
        : base_t(
              std::move(socket),
              cluster_bus,
              fabric,
              logger,
              imega::parser(
                  [=, this](std::uint8_t protocol, cyng::version v, std::string telnb, std::string mname) {
                      //  login
                      switch (policy) {
                      case imega::policy::MNAME:
                          CYNG_LOG_INFO(
                              logger_,
                              "[pty] login MNAME " << vm_.get_tag() << ", " << mname << ':' << telnb << '@'
                                                   << get_remote_endpoint());
                          bus_.pty_login(
                              mname, mname, vm_.get_tag(), config::get_name(config::protocol::DLMS), get_remote_endpoint());
                          break;
                      case imega::policy::TELNB:
                          CYNG_LOG_INFO(
                              logger_,
                              "[pty] login TELNB " << vm_.get_tag() << ", " << mname << ':' << telnb << '@'
                                                   << get_remote_endpoint());
                          bus_.pty_login(
                              telnb, mname, vm_.get_tag(), config::get_name(config::protocol::DLMS), get_remote_endpoint());
                          break;
                      default:
                          //    global
                          CYNG_LOG_INFO(
                              logger_,
                              "[pty] login GLOBAL " << vm_.get_tag() << ", " << mname << ':' << telnb << ':' << pwd << '@'
                                                    << get_remote_endpoint());
                          bus_.pty_login(
                              mname, pwd, vm_.get_tag(), config::get_name(config::protocol::DLMS), get_remote_endpoint());
                          break;
                      }
                  },
                  [&, this]() {
                      //  watchdog
                      CYNG_LOG_TRACE(logger_, "[pty] watchdog " << vm_.get_tag() << " from " << get_remote_endpoint());
                  },
                  [&, this](cyng::buffer_t &&data, bool ok) {
                      if (ok) {
                          //    transfer data over open connection
                          CYNG_LOG_TRACE(
                              logger_, "[pty] data " << vm_.get_tag() << " from " << get_remote_endpoint() << ": " << data);
                          bus_.pty_transfer_data(this->dev_, this->vm_.get_tag(), std::move(data));
                      } else {
                          CYNG_LOG_WARNING(
                              logger_, "[pty] data " << vm_.get_tag() << " from " << get_remote_endpoint() << ": " << data);
                      }
                  }),
              imega::serializer{}) {

        //
        //  initialize VM
        //
        vm_ = init_vm(fabric);

        CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << get_remote_endpoint() << " created");
    }

    imega_session::~imega_session() {
#ifdef _DEBUG_IMEGA
        std::cout << "session(~)" << std::endl;
#endif
    }

    cyng::vm_proxy imega_session::init_vm(cyng::mesh &fabric) {
        return fabric.make_proxy(
            bus_.get_tag(),
            //  handle dispatch errors
            std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2),
            cyng::make_description(
                "pty.res.login",
                cyng::vm_adaptor<imega_session, void, bool, boost::uuids::uuid>(this, &imega_session::pty_res_login)),
            // "pty.res.register" is not implemented
            // "pty.res.open.channel" is not implemented
            // "pty.req.push.data" is not implemented
            // "pty.push.data.res" is not implemented
            // "pty.res.close.channel" is not implemented
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
    }

    void imega_session::stop() {
        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
        if (vm_.stop()) {
            close_socket();
        }
    }

    void imega_session::logout() { bus_.pty_logout(dev_, vm_.get_tag()); }

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
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, data.begin(), data.end());
            auto const dmp = ss.str();
            CYNG_LOG_TRACE(logger_, "[" << get_remote_endpoint() << "] emit " << data.size() << " bytes:\n" << dmp);
        }
        //  send to device
        send([=, this]() mutable -> cyng::buffer_t { return serializer_.raw_data(std::move(data)); });
    }

    void imega_session::pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token) {

        CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " req open connection " << msisdn);
        //  auto response
        bus_.pty_res_open_connection(true, dev_, vm_.get_tag(), std::move(token));
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
