/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <router.h>
#include <storage.h>

#include <config/cfg_hardware.h>
#include <config/cfg_ipt.h>
#include <config/cfg_sml.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/transpiler.h>
#include <smf/sml/reader.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

#include <functional>

namespace smf {

    router::router(cyng::logger logger, cyng::controller &ctl, cfg &config, storage &store)
        : logger_(logger)
        , ctl_(ctl)
        , cfg_(config)
        , bus_()
        , parser_([this](
                      std::string trx,
                      std::uint8_t group_no,
                      std::uint8_t abort_on_error,
                      sml::msg_type type,
                      cyng::tuple_t msg,
                      std::uint16_t crc) {
            CYNG_LOG_TRACE(logger_, smf::sml::get_name(type) << ": " << trx << ", " << msg);
            //
            //  std::apply() needs as first parameter a this pointer because generate_open_response()
            //  is a member function.
            //
            switch (type) {
            case sml::msg_type::OPEN_REQUEST:
                std::apply(
                    &router::generate_open_response,
                    std::tuple_cat(std::make_tuple(this, trx), sml::read_public_open_request(msg)));
                break;
            case sml::msg_type::GET_PROC_PARAMETER_REQUEST:
                std::apply(
                    &response_engine::generate_get_proc_parameter_response,
                    std::tuple_cat(std::make_tuple(&engine_, std::ref(messages_), trx), sml::read_get_proc_parameter_request(msg)));
                break;
            case sml::msg_type::SET_PROC_PARAMETER_REQUEST:
                std::apply(
                    &response_engine::generate_set_proc_parameter_response,
                    std::tuple_cat(std::make_tuple(&engine_, std::ref(messages_), trx), sml::read_set_proc_parameter_request(msg)));
                break;
            case sml::msg_type::GET_PROFILE_LIST_REQUEST:
                std::apply(
                    &response_engine::generate_get_profile_list_response,
                    std::tuple_cat(std::make_tuple(&engine_, std::ref(messages_), trx), sml::read_get_profile_list_request(msg)));
                break;
            case sml::msg_type::CLOSE_REQUEST: generate_close_response(trx, sml::read_public_close_request(msg)); break;
            default: CYNG_LOG_WARNING(logger_, "message type " << smf::sml::get_name(type) << " is not supported yet"); break;
            }
            //
            //  send response
            //
            reply();
        })
        , messages_()
        , res_gen_()
        , engine_(logger, cfg_, store, res_gen_) {}

    void router::start() {

        cfg_ipt ipt_cfg(cfg_);
        if (ipt_cfg.is_enabled()) {
            //
            //	start IP-T bus
            //
            cfg_hardware hw_cfg(cfg_);
            CYNG_LOG_INFO(logger_, "[ipt bus] initialize as " << hw_cfg.get_model());
            try {
                bus_ = std::make_unique<ipt::bus>(
                    ctl_.get_ctx(),
                    logger_,
                    ipt_cfg.get_toggle(),
                    hw_cfg.get_model(),
                    std::bind(&router::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&router::ipt_stream, this, std::placeholders::_1),
                    std::bind(&router::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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
        CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte");

#ifdef _DEBUG_SEGW
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, std::begin(data), std::end(data));
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[ipt] stream " << data.size() << " bytes:\n" << dmp);
        }
#endif

        parser_.read(data.begin(), data.end());
    }

    void router::auth_state(bool auth, boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep) {
        //
        //  update status word
        //
        cfg_.update_status_word(sml::status_bit::NOT_AUTHORIZED_IPT, !auth);
        ctl_.get_registry().dispatch("persistence", "oplog.authorized", auth);

        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized at " << rep);
            cfg_ipt ipt_cfg(cfg_);
            ipt_cfg.set_local_enpdoint(lep);
            ipt_cfg.set_remote_enpdoint(rep);
            register_targets();
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
        }
    }

    void router::register_targets() {
        //
        //  check for targets to register
        //
    }

    void router::reply() {

        //
        //  convert to SML
        //
        auto buffer = sml::to_sml(messages_);

#ifdef _DEBUG_SEGW
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, std::begin(buffer), std::end(buffer));
            auto const dmp = ss.str();
            CYNG_LOG_DEBUG(logger_, "[ipt] response " << buffer.size() << " bytes:\n" << dmp);
        }
#endif

        //
        //  send
        //
        if (bus_) {
            bus_->transfer(std::move(buffer));
        }

        //
        //  clear message buffer
        //
        messages_.clear();
    }

    void router::generate_open_response(
        std::string trx,
        std::string code_page,
        cyng::buffer_t file_id,
        cyng::buffer_t client,
        std::string server,
        std::string user,
        std::string pwd,
        std::uint8_t version) {

        CYNG_LOG_TRACE(
            logger_,
            "SML_PublicOpen.Req - trx: " << trx << ", client: " << cyng::io::to_hex(client) << ", server: " << server
                                         << ", file id: " << cyng::io::to_hex(file_id) << ", user: " << user << ", pwd: " << pwd);

        cfg_sml cfg(cfg_);
        if (cfg.accept_all_ids()) {
            CYNG_LOG_WARNING(logger_, "[ipt] accept all ids");
        } else {
        }

        messages_.push_back(res_gen_.public_open(trx, file_id, client, server));
    }

    void router::generate_close_response(std::string trx, cyng::object gsign) { messages_.push_back(res_gen_.public_close(trx)); }

} // namespace smf
