/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/push.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/transpiler.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
#include <smf/sml/crc16.h>
#include <smf/sml/serializer.h>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif
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
        std::bind(&push::connect, this), // "connect"
        std::bind(&push::send_sml, this, std::placeholders::_1, std::placeholders::_2), // "send.sml"
        std::bind(&push::send_mbus, this, std::placeholders::_1, std::placeholders::_2), // "send.mbus"
        std::bind(&push::send_dlms, this), // "send.dlms"
        std::bind(
            &push::on_channel_open,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5), // "on.channel.open"
        std::bind(
            &push::on_channel_close,
            this,
            std::placeholders::_1,
            std::placeholders::_2), // "on.channel.close"
        std::bind(&push::open_push_channels, this), // "open.channels"
        std::bind(&push::stop, this, std::placeholders::_1),
    }
    , channel_(wp), logger_(logger), pcc_sml_(pcc_sml), pcc_iec_(pcc_iec), pcc_dlms_(pcc_dlms)
    , bus_(
        ctl,
        logger,
        std::move(cfg),
        "model",
        std::bind(&push::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&push::ipt_stream, this, std::placeholders::_1),
        std::bind(&push::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , channels_()
    , sml_generator_()
    , trx_(7)
    {
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names(
                {"connect", "send.sml", "send.mbus", "send.dlms", "on.channel.open", "on.channel.close", "open.channels"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
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
        auto pos = channels_.find(config::protocol::SML);
        if (pos != channels_.end()) {
            CYNG_LOG_INFO(
                logger_,
                "[wmbus.push] sml " << payload.size() << " bytes payload from " << srv_id_to_str(srv, true) << " to "
                                    << pos->second.first << ':' << pos->second.second);
            bus_.transmit(pos->second, payload);
        } else {
            CYNG_LOG_WARNING(logger_, "[wmbus.push] no SML channel open");
        }

        //
        //  generate SML close response message
        //
    }
    void push::send_mbus(cyng::buffer_t srv, cyng::tuple_t sml_list) {
        CYNG_LOG_INFO(logger_, "[wmbus.push] mbus data");

        //
        //  generate SML open response message
        //

        //
        //  generate SML message
        //
        auto const trx = *++trx_;
        auto const tpl = sml_generator_.get_list(
            trx,                  // trx
            cyng::buffer_t{},     // client
            srv,                  // meter id
            OBIS_PROFILE_INITIAL, // profile
            sml_list,             // data
            0                     // seconds index
        );
        auto msg = sml::set_crc16(sml::serialize(tpl));
        auto const payload = sml::boxing(std::move(msg));

#ifdef _DEBUG
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, payload.begin(), payload.end());
            auto const dmp = ss.str();
            CYNG_LOG_TRACE(logger_, "[synthetic SML_GetList.Res] :\n" << dmp);
        }
#endif

        //
        //  generate SML close response message
        //

        //
        //  send payload
        //

        auto pos = channels_.find(config::protocol::SML);
        if (pos != channels_.end()) {
            CYNG_LOG_INFO(
                logger_,
                "[wmbus.push] mbus/sml " << payload.size() << " bytes payload from " << srv_id_to_str(srv, true) << " to "
                                         << pos->second.first << ':' << pos->second.second);
            bus_.transmit(pos->second, payload);
        } else {
            CYNG_LOG_WARNING(logger_, "[wmbus.push] no SML channel open");
        }
    }

    void push::send_dlms() {
        CYNG_LOG_INFO(logger_, "[wmbus.push] dlms data");

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

        switch (ipt::to_code(h.command_)) {
        case ipt::code::TP_RES_PUSHDATA_TRANSFER: {
            auto [ok, res, channel, source, status, block] = ipt::tp_res_pushdata_transfer(std::move(body));
            if (ipt::tp_res_pushdata_transfer_policy::is_success(res)) {
                CYNG_LOG_TRACE(
                    logger_,
                    "[wmbus.push] transfer [" << channel << "," << source
                                              << "]: " << ipt::tp_res_pushdata_transfer_policy::get_response_name(res));

            } else {
                CYNG_LOG_WARNING(
                    logger_,
                    "[wmbus.push] transfer [" << channel << "," << source
                                              << "]: " << ipt::tp_res_pushdata_transfer_policy::get_response_name(res));
                //  close/reopen channel
                for (auto const &data : channels_) {
                    CYNG_LOG_WARNING(
                        logger_,
                        "close channel " << config::get_name(data.first) << " [" << data.second.first << ',' << data.second.second
                                         << "]");
                    //
                    //  Since the channel is broken, closing the channel will probably not work.
                    //
                    bus_.close_channel(data.second.first, channel_);
                }

                //
                //  clear channel table
                //
                channels_.clear();

                if (auto sp = channel_.lock(); sp) {
                    CYNG_LOG_INFO(logger_, "[wmbus.push] reopen channels in one minute");
                    //  handle dispatch errors
                    sp->suspend(
                        std::chrono::minutes(1),
                        "open.channels",
                        std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
                } else {
                    CYNG_LOG_ERROR(logger_, "[wmbus.push] channel invalid - cannot reopen channels");
                }
            }
        } break;

        default: CYNG_LOG_TRACE(logger_, "[wmbus.push] cmd " << ipt::command_name(h.command_)); break;
        }
    }

    void push::ipt_stream(cyng::buffer_t &&data) {
        //
        //  There should _not_ be an ipt data stream
        //
        CYNG_LOG_WARNING(logger_, "[ipt] stream " << data.size() << " byte");
    }

    void push::auth_state(bool auth, boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized at " << rep);
            BOOST_ASSERT(bus_.is_authorized());
            //
            //  open push channel
            //
            open_push_channels();
        } else {
            CYNG_LOG_WARNING(logger_, "[wmbus.push] authorization lost");
            channels_.clear();

            //
            //  reconnect in 1 minute
            //
            if (auto sp = channel_.lock(); sp) {
                CYNG_LOG_INFO(logger_, "[wmbus.push] reconnect in one minute");
                //  handle dispatch errors
                sp->suspend(
                    std::chrono::minutes(1),
                    "connect",
                    std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
            } else {
                CYNG_LOG_ERROR(logger_, "[wmbus.push] channel invalid - cannot reconnect");
            }
        }
    }

    void push::open_push_channels() {
        CYNG_LOG_INFO(logger_, "[wmbus.push] open SML channel " << pcc_sml_);
        bus_.open_channel(pcc_sml_, channel_);

        // std::this_thread::sleep_for(std::chrono::seconds(1));
        CYNG_LOG_INFO(logger_, "[wmbus.push] open IEC channel " << pcc_iec_);
        bus_.open_channel(pcc_iec_, channel_);

        // std::this_thread::sleep_for(std::chrono::seconds(1));
        CYNG_LOG_INFO(logger_, "[wmbus.push] open DLMS channel " << pcc_dlms_);
        bus_.open_channel(pcc_dlms_, channel_);
    }

    void push::on_channel_open(bool success, std::uint32_t channel, std::uint32_t source, std::uint32_t count, std::string target) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[wmbus.push] channel " << target << " is open #" << channel << ':' << source);

            //
            //  update channel list
            // protocol_type { SML, IEC, DLMS };
            //
            if (boost::algorithm::equals(target, pcc_sml_.target_)) {
                channels_.emplace(config::protocol::SML, std::make_pair(channel, source));
            } else if (boost::algorithm::equals(target, pcc_iec_.target_)) {
                channels_.emplace(config::protocol::IEC, std::make_pair(channel, source));
            } else if (boost::algorithm::equals(target, pcc_dlms_.target_)) {
                channels_.emplace(config::protocol::DLMS, std::make_pair(channel, source));
            } else {
                CYNG_LOG_WARNING(logger_, "[wmbus.push] unknown push channel: " << target);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[wmbus.push] open push channel failed");
        }
    }

    void push::on_channel_close(bool success, std::uint32_t channel) {
        if (success) {
            CYNG_LOG_TRACE(logger_, "[wmbus.push] channel " << channel << " closed");
        } else {
            //  This is expected: see remark in push::ipt_cmd()
            CYNG_LOG_INFO(logger_, "[wmbus.push] channel " << channel << " could not be closed (this is expected)");
        }
    }

} // namespace smf
