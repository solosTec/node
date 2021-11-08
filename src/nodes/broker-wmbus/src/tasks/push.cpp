/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/push.h>

#include <smf/mbus/server_id.h>
#include <smf/sml/crc16.h>
#include <smf/sml/serializer.h>

#include <cyng/io/hex_dump.hpp>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>

namespace smf {

    push::push(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        ipt::toggle::server_vec_t &&cfg,
        ipt::push_channel &&pcc_sml,
        ipt::push_channel &&pcc_iec,
        ipt::push_channel &&pcc_dlms)
    : sigs_ {
        std::bind(&push::connect, this), 
        std::bind(&push::send_sml, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&push::send_mbus, this, std::placeholders::_1, std::placeholders::_2), 
        std::bind(&push::send_dlms, this),
        std::bind(
            &push::on_channel_open,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5),

        std::bind(&push::stop, this, std::placeholders::_1),
    }
    , channel_(wp), logger_(logger), pcc_sml_(pcc_sml), pcc_iec_(pcc_iec), pcc_dlms_(pcc_dlms)
    , bus_(
        ctl.get_ctx(),
        logger,
        std::move(cfg),
        "model",
        std::bind(&push::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&push::ipt_stream, this, std::placeholders::_1),
        std::bind(&push::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , channels_()
    , sml_generator_()
    {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names({"connect", "send.sml", "send.mbus", "send.dlms", "channel.open"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    push::~push() {
#ifdef _DEBUG_BROKER_WMBUS
        std::cout << "push(~)" << std::endl;
#endif
    }

    void push::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop push task");
        bus_.stop();
    }

    void push::connect() {
        //
        //	start IP-T bus
        //
        CYNG_LOG_INFO(logger_, "initialize: IP-T client");
        bus_.start();
    }

    void push::send_sml(cyng::buffer_t srv, cyng::buffer_t payload) {

        //
        //  generate SML open response message
        //

        //
        //  send payload
        // protocol_type { SML, IEC, DLMS };
        //
        auto pos = channels_.find(protocol_type::SML);
        if (pos != channels_.end()) {
            CYNG_LOG_INFO(
                logger_,
                "[push] sml " << payload.size() << " bytes payload from " << srv_id_to_str(srv) << " to " << pos->second.first
                              << ':' << pos->second.second);
            bus_.transmit(pos->second, payload);
        } else {
            CYNG_LOG_WARNING(logger_, "[push] no SML channel open");
        }

        //
        //  generate SML close response message
        //
    }
    void push::send_mbus(cyng::buffer_t srv, cyng::tuple_t sml_list) {
        CYNG_LOG_INFO(logger_, "[push] mbus data");

        //
        //  generate SML open response message
        //

        //
        //  generate SML  message
        //
        auto const tpl = sml_generator_.get_list(srv, sml_list);
        auto const msg = sml::set_crc16(sml::serialize(tpl));
        auto const payload = sml::boxing(msg);

        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, payload.begin(), payload.end());
            auto const dmp = ss.str();
            CYNG_LOG_TRACE(logger_, "[synthetic SML_GetList.Res] :\n" << dmp);
        }

        //
        //  generate SML close response message
        //

        //
        //  send payload
        //

        auto pos = channels_.find(protocol_type::SML);
        if (pos != channels_.end()) {
            CYNG_LOG_INFO(
                logger_,
                "[push] mbus/sml " << payload.size() << " bytes payload from " << srv_id_to_str(srv) << " to " << pos->second.first
                                   << ':' << pos->second.second);
            bus_.transmit(pos->second, payload);
        } else {
            CYNG_LOG_WARNING(logger_, "[push] no SML channel open");
        }
    }

    void push::send_dlms() {
        CYNG_LOG_INFO(logger_, "[push] dlms data");

        //
        //  generate SML open response message
        //

        //
        //  send payload
        //

        //
        //  generate SML close response message
        //

        //
        //  close message
        //
    }

    void push::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {

        CYNG_LOG_TRACE(logger_, "[push] cmd " << ipt::command_name(h.command_));
    }
    void push::ipt_stream(cyng::buffer_t &&data) { CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte"); }

    void push::auth_state(bool auth, boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized at " << rep);
            BOOST_ASSERT(bus_.is_authorized());
            //
            //  open push channel
            //
            open_push_channels();
        } else {
            CYNG_LOG_WARNING(logger_, "[push] authorization lost");
            channels_.clear();

            //
            //  reconnect in 1 minute
            //
            auto sp = channel_.lock();
            if (sp) {
                sp->suspend(std::chrono::minutes(1), "connect");
            }
        }
    }

    void push::open_push_channels() {
        bus_.open_channel(pcc_sml_, channel_);
        bus_.open_channel(pcc_iec_, channel_);
        bus_.open_channel(pcc_dlms_, channel_);
    }

    void push::on_channel_open(bool success, std::uint32_t channel, std::uint32_t source, std::uint32_t count, std::string target) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel " << target << " is open #" << channel << ':' << source);

            //
            //  update channel list
            // protocol_type { SML, IEC, DLMS };
            //
            if (boost::algorithm::equals(target, pcc_sml_.target_)) {
                channels_.emplace(protocol_type::SML, std::make_pair(channel, source));
            } else if (boost::algorithm::equals(target, pcc_iec_.target_)) {
                channels_.emplace(protocol_type::IEC, std::make_pair(channel, source));
            } else if (boost::algorithm::equals(target, pcc_dlms_.target_)) {
                channels_.emplace(protocol_type::DLMS, std::make_pair(channel, source));
            } else {
                CYNG_LOG_WARNING(logger_, "[push] unknown push channel: " << target);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[push] open push channel failed");
        }
    }

} // namespace smf
