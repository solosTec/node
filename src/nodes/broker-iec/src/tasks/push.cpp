/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>
#include <tasks/push.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    push::push(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, ipt::toggle::server_vec_t&& cfg
		, ipt::push_channel&& pcc)
	: sigs_{ 
		std::bind(&push::connect, this),  
        std::bind(&push::send_iec, this, std::placeholders::_1),
        std::bind(
            &push::on_channel_open,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4),
		std::bind(&push::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, logger_(logger)
		, pcc_(pcc)
		, bus_(ctl.get_ctx()
			, logger
			, std::move(cfg)
			, "model"
			, std::bind(&push::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&push::ipt_stream, this, std::placeholders::_1)
            
			, std::bind(&push::auth_state, this, std::placeholders::_1))
        , id_(0u, 0u)
	{
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("connect", slot++);
            sp->set_channel_name("send.iec", slot++);
            sp->set_channel_name("channel.open", slot++);
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

    void push::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {

        CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));
    }
    void push::ipt_stream(cyng::buffer_t &&data) { CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte"); }

    void push::auth_state(bool auth) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized");
            BOOST_ASSERT(bus_.is_authorized());
            bus_.open_channel(pcc_, channel_);
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
        }
    }

    void push::on_channel_open(std::uint32_t channel, std::uint32_t source, std::uint32_t count, std::string target) {
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
    }

    void push::send_iec(cyng::buffer_t payload) { bus_.transmit(id_, payload); }

} // namespace smf
