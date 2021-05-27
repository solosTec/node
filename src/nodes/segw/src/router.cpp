/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_hardware.h>
#include <config/cfg_ipt.h>
#include <router.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/transpiler.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    router::router(cyng::controller &ctl, cfg &config, cyng::logger logger)
        : ctl_(ctl),
          cfg_(config),
          logger_(logger),
          bus_(),
          parser_([this](std::string trx, std::uint8_t, std::uint8_t,
                         smf::sml::msg_type type, cyng::tuple_t msg,
                         std::uint16_t crc) {
              CYNG_LOG_TRACE(logger_, smf::sml::get_name(type)
                                          << ": " << trx << ", " << msg);
              switch (type) {
              case sml::msg_type::OPEN_REQUEST:
                  break;
              case sml::msg_type::GET_PROC_PARAMETER_REQUEST:
                  break;
              case sml::msg_type::CLOSE_REQUEST:
                  break;
              default:
                  CYNG_LOG_WARNING(logger_, "message type "
                                                << smf::sml::get_name(type)
                                                << " is not supported yet");
                  break;
              }
          }),
          messages_() {}

    void router::start() {

        cfg_ipt ipt_cfg(cfg_);
        if (ipt_cfg.is_enabled()) {
            //
            //	start IP-T bus
            //
            cfg_hardware hw_cfg(cfg_);
            CYNG_LOG_INFO(logger_,
                          "[ipt bus] initialize as " << hw_cfg.get_model());
            try {
                bus_ = std::make_unique<ipt::bus>(
                    ctl_.get_ctx(), logger_, ipt_cfg.get_toggle(),
                    hw_cfg.get_model(),
                    std::bind(&router::ipt_cmd, this, std::placeholders::_1,
                              std::placeholders::_2),
                    std::bind(&router::ipt_stream, this, std::placeholders::_1),
                    std::bind(&router::auth_state, this,
                              std::placeholders::_1));
                bus_->start();
            } catch (std::exception const &ex) {
                CYNG_LOG_ERROR(logger_, "[ipt bus] start failed " << ex.what());
                bus_.reset();
            }

            //
            //	start OBIS log
            //
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt bus] not enabled");
        }
    }

    void router::stop() {
        cfg_ipt ipt_cfg(cfg_);
        if (ipt_cfg.is_enabled()) {
            if (bus_) {
                CYNG_LOG_INFO(logger_, "[ipt bus] stop");
                bus_->stop();
                bus_.reset();
            } else {
                CYNG_LOG_WARNING(logger_, "[ipt bus] is null");
            }
        } else {
            CYNG_LOG_TRACE(logger_, "[ipt bus] not enabled");
        }
    }

    void router::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {

        CYNG_LOG_INFO(logger_, "[ipt]  cmd " << ipt::command_name(h.command_));
    }

    void router::ipt_stream(cyng::buffer_t &&data) {
        CYNG_LOG_TRACE(logger_, "[ipt]  stream " << data.size() << " byte");

#ifdef _DEBUG_SEGW
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, std::begin(data), std::end(data));
            CYNG_LOG_DEBUG(logger_, "[ipt]  stream " << data.size()
                                                     << " bytes:\n"
                                                     << ss.str());
        }
#endif

        parser_.read(data.begin(), data.end());
    }

    void router::auth_state(bool auth) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized");
            register_targets();
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
        }
    }

    void router::register_targets() {}

} // namespace smf
