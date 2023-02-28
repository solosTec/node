/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/push.h>

#include <smf/ipt/response.hpp>
#include <smf/ipt/transpiler.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    push::push(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, ipt::toggle::server_vec_t&& cfg
		, ipt::push_channel const& pcc
        , std::string const &client)
	: sigs_{ 
        std::bind(&push::connect, this),  // "connect"
        std::bind(&push::open, this), // "open"
        std::bind(&push::close, this), // "close"
        std::bind(&push::forward, this, std::placeholders::_1), // "push"
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
		std::bind(&push::stop, this, std::placeholders::_1), // stop
	}
		, channel_(wp)
        , registry_(ctl.get_registry())
		, logger_(logger)
		, pcc_(pcc)
        , client_task_(client)
		, bus_(ctl
			, logger
			, std::move(cfg)
			, "IEC-Broker"
			, std::bind(&push::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&push::ipt_stream, this, std::placeholders::_1)           
			, std::bind(&push::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
        , id_(0u, 0u)
        , buffer_write_()
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect", "open", "close", "push", "on.channel.open", "on.channel.close"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void push::stop(cyng::eod) {
        // CYNG_LOG_WARNING(logger_, "stop push task");
        bus_.stop();
    }

    void push::open() {
        auto sp = channel_.lock();
        BOOST_ASSERT(bus_.is_authorized());
        if (sp) {
            if (bus_.is_authorized()) {
                CYNG_LOG_WARNING(logger_, "[iec.push] " << sp->get_name() << " opens push channel \"" << pcc_.target_ << "\"");
                bus_.open_channel(pcc_, channel_);
            } else {
                CYNG_LOG_WARNING(logger_, "[iec.push] " << sp->get_name() << " not authorized");
            }
        }
    }

    void push::connect() {
        //
        //	start IP-T bus
        //
        CYNG_LOG_INFO(logger_, "[iec.push] initialize");
        bus_.start();
    }

    void push::close() { bus_.close_channel(ipt::get_channel(id_), channel_); }

    void push::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {

        switch (ipt::to_code(h.command_)) {
        case ipt::code::TP_RES_PUSHDATA_TRANSFER: {
            auto [ok, res, channel, source, status, block] = ipt::tp_res_pushdata_transfer(std::move(body));
            if (ok) {
                if (ipt::tp_res_pushdata_transfer_policy::is_success(res)) {
                    CYNG_LOG_TRACE(
                        logger_,
                        "[iec.push] transfer [" << channel << "," << source
                                                << "]: " << ipt::tp_res_pushdata_transfer_policy::get_response_name(res));

                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "[iec.push] transfer [" << channel << "," << source
                                                << "]: " << ipt::tp_res_pushdata_transfer_policy::get_response_name(res));
                    //  close/reopen channel

                    // if (auto sp = channel_.lock(); sp) {
                    //     CYNG_LOG_INFO(logger_, "[push] reopen channels in one minute");
                    //     sp->suspend(std::chrono::minutes(1), "open.channels");
                    // }
                    // else {
                    //     CYNG_LOG_ERROR(logger_, "[push] channel invalid - cannot reopen channels");
                    // }
                }
            } else {
                CYNG_LOG_ERROR(logger_, "[iec.push] invalid push data transfer response");
            }
        } break;

        default: CYNG_LOG_TRACE(logger_, "[iec.push] cmd " << ipt::command_name(h.command_)); break;
        }

        // CYNG_LOG_TRACE(logger_, "[iec.push] cmd " << ipt::command_name(h.command_));
    }
    void push::ipt_stream(cyng::buffer_t &&data) { CYNG_LOG_TRACE(logger_, "[iec.push] stream " << data.size() << " byte"); }

    void push::auth_state(bool auth, boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[iec.push] authorized at " << rep);
            // BOOST_ASSERT(bus_.is_authorized());
        } else {
            CYNG_LOG_WARNING(logger_, "[iec.push] authorization lost");

            //
            //  bus automatically reconnects
            //
        }
    }

    void push::on_channel_open(bool success, std::uint32_t channel, std::uint32_t source, std::uint32_t count, std::string target) {

        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel " << target << " is open #" << channel << ':' << source);

            //
            //  update channel list
            // protocol_type { SML, IEC, DLMS };
            //
            if (boost::algorithm::equals(target, pcc_.target_)) {
                id_ = std::make_pair(channel, source);
            } else {
                CYNG_LOG_WARNING(logger_, "[push] unknown push channel: " << target);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[push] open push channel failed");
        }

        //
        //  send update status to client
        //
        if (auto sp = channel_.lock(); sp) {
            registry_.dispatch(client_task_, "channel.open", success, sp->get_name(), channel, source);
        }
    }

    void push::on_channel_close(bool success, std::uint32_t channel) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel " << channel << " is closed");
            auto sp = channel_.lock();
            if (sp) {
                registry_.dispatch(client_task_, "channel.close", sp->get_name(), channel);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[push] close channel " << channel << " failed");
        }
        //
        //  reset push channel
        //
        ipt::init(id_);
        buffer_write_.clear();
    }
    void push::send_iec(cyng::buffer_t payload) { bus_.transmit(id_, payload); }
    void push::forward(cyng::buffer_t payload) {
        if (bus_.is_authorized()) {
            if (ipt::is_null(id_)) {
                buffer_write_.push_back(payload);
                bus_.open_channel(pcc_, channel_);
            } else {
                send_iec(payload);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[push] not authorized: " << channel_.lock()->get_name());
        }
    }

} // namespace smf
