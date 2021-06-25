/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/push.h>

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
		, ipt::push_channel const& pcc)
	: sigs_{ 
		std::bind(&push::connect, this),  
        std::bind(&push::close, this),
        std::bind(&push::forward, this, std::placeholders::_1),
        std::bind(
            &push::on_channel_open,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5),
        std::bind(
            &push::on_channel_close,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
		std::bind(&push::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, logger_(logger)
		, pcc_(pcc)
		, bus_(ctl.get_ctx()
			, logger
			, std::move(cfg)
			, "IEC-Broker"
			, std::bind(&push::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&push::ipt_stream, this, std::placeholders::_1)           
			, std::bind(&push::auth_state, this, std::placeholders::_1))
        , id_(0u, 0u)
        , buffer_write_()
	{
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("connect", slot++);
            sp->set_channel_name("close", slot++);
            sp->set_channel_name("push", slot++); //  forward
            sp->set_channel_name("channel.open", slot++);
            sp->set_channel_name("channel.close", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    push::~push() {
#ifdef _DEBUG_BROKER_WMBUS
        std::cout << "push(~)" << std::endl;
#endif
    }

    void push::stop(cyng::eod) {
        // CYNG_LOG_WARNING(logger_, "stop push task");
        bus_.stop();
    }

    void push::connect() {
        //
        //	start IP-T bus
        //
        CYNG_LOG_INFO(logger_, "initialize: IP-T client");
        bus_.start();
    }

    void push::close() { bus_.close_channel(ipt::get_channel(id_), channel_); }

    void push::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {

        CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));
    }
    void push::ipt_stream(cyng::buffer_t &&data) { CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte"); }

    void push::auth_state(bool auth) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized");
            // BOOST_ASSERT(bus_.is_authorized());
            bus_.open_channel(pcc_, channel_);
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");

            //
            //  reset push channel
            //
            ipt::init(id_);
            buffer_write_.clear();

            //
            //  reconnect in 1 minute
            //
            auto sp = channel_.lock();
            if (sp) {
                sp->suspend(std::chrono::minutes(1), "connect");
            }
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
                if (!buffer_write_.empty()) {
                    for (auto const &payload : buffer_write_) {
                        send_iec(payload);
                    }
                    buffer_write_.clear();
                }
            } else {
                CYNG_LOG_WARNING(logger_, "[push] unknown push channel: " << target);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[push] open push channel failed");
        }
    }

    void push::on_channel_close(bool success, std::uint32_t channel) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[push] channel " << channel << " is closed");
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
