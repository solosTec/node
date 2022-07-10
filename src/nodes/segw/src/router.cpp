/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <router.h>

#include <storage.h>
#include <tasks/push.h>
#include <tasks/store.h>

#include <config/cfg_sml.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/transpiler.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>
#include <smf/sml/reader.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/store/db.h>
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
        , ipt_cfg_(config)
        , hw_cfg_(config)
        , bus_(
              ctl_.get_ctx(),
              logger_,
              ipt_cfg_.get_toggle(),
              hw_cfg_.get_model(),
              std::bind(&router::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&router::ipt_stream, this, std::placeholders::_1),
              std::bind(&router::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
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
            case sml::msg_type::GET_LIST_REQUEST:
                std::apply(
                    &response_engine::generate_get_list_response,
                    std::tuple_cat(std::make_tuple(&engine_, std::ref(messages_), trx), sml::read_get_list_request(msg)));
                break;
            case sml::msg_type::CLOSE_REQUEST:
                generate_close_response(trx, sml::read_public_close_request(msg));
                //
                //  send response
                //
                reply();
                break;
            default: CYNG_LOG_WARNING(logger_, "message type " << smf::sml::get_name(type) << " is not supported yet"); break;
            }
        })
        , messages_()
        , res_gen_()
        , engine_(logger, ctl_, cfg_, store, bus_, res_gen_) {}

    bool router::start() {

        if (ipt_cfg_.is_enabled()) {
            //
            //	start IP-T bus
            //
            CYNG_LOG_INFO(logger_, "[ipt bus] initialize as " << hw_cfg_.get_model());
            try {
                // bus_ = std::make_unique<ipt::bus>(
                //     ctl_.get_ctx(),
                //     logger_,
                //     ipt_cfg.get_toggle(),
                //     hw_cfg.get_model(),
                //     std::bind(&router::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
                //     std::bind(&router::ipt_stream, this, std::placeholders::_1),
                //     std::bind(&router::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                bus_.start();

                init_ipt_push();
                init_ipt_store();

                return true;

            } catch (std::exception const &ex) {
                CYNG_LOG_ERROR(logger_, "[ipt bus] start failed " << ex.what());
                // bus_.reset();
            }
        }
        CYNG_LOG_WARNING(logger_, "[ipt bus] not enabled");
        return false;
    }

    void router::stop() {

        if (ipt_cfg_.is_enabled()) {
            stop_ipt_push();
            stop_ipt_store();
            CYNG_LOG_INFO(logger_, "[ipt bus] stop");
            bus_.stop();
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
            ipt_cfg_.set_local_enpdoint(lep);
            ipt_cfg_.set_remote_enpdoint(rep);
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
        if (bus_.is_authorized()) {
            bus_.transfer(std::move(buffer));
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

    void router::init_ipt_push() {

        //
        //  start all push tasks
        //
        cfg_.get_cache().access(
            [&, this](cyng::table *tbl) {
                //
                //  store task ids
                //
                std::map<cyng::key_t, std::size_t> task_ids;
                tbl->loop([&, this](cyng::record &&rec, std::size_t) -> bool {
                    auto const key = rec.key();
                    auto const profile = rec.value("profile", OBIS_PROFILE);
                    auto const interval = rec.value("interval", std::chrono::seconds(0));
                    auto const delay = rec.value("delay", std::chrono::seconds(0));
                    auto const target = rec.value("target", "");
                    CYNG_LOG_INFO(logger_, "[ipt] initialize: \"push\" task [" << target << "], " << obis::get_name(profile));

                    auto channel = ctl_.create_named_channel_with_ref<push>("push", logger_, bus_, cfg_, key);
                    BOOST_ASSERT(channel->is_open());
                    channel->dispatch("init");
                    task_ids.emplace(key, channel->get_id());
                    return true;
                });

                //
                //  update task ids
                //
                for (auto const &v : task_ids) {
                    tbl->modify(v.first, cyng::make_param("task", v.second), cfg_.get_tag());
                }
            },
            cyng::access::write("pushOps"));
    }
    void router::stop_ipt_push() {
        //
        //  stops all tasks with this name
        //
        CYNG_LOG_INFO(logger_, "[ipt] stop \"push\" tasks");
        ctl_.get_registry().stop("push");
    }

    void router::init_ipt_store() {
        //
        //  start collecting push data
        //
        for (auto const profile : sml::get_profiles()) {
            CYNG_LOG_INFO(
                logger_, "[ipt] start \"store\" task " << obis::get_name(profile) << " (" << cyng::to_string(profile) << ")");
            auto channel = ctl_.create_named_channel_with_ref<store>(cyng::to_string(profile), logger_, bus_, cfg_, profile);
            BOOST_ASSERT(channel->is_open());
            channel->dispatch("init");
        }
    }
    void router::stop_ipt_store() {
        //
        //  stop collecting push data
        //
        for (auto const profile : sml::get_profiles()) {
            CYNG_LOG_INFO(logger_, "[ipt] stop \"store\" task " << obis::get_name(profile));
            ctl_.get_registry().stop(cyng::to_string(profile));
        }
    }

} // namespace smf
