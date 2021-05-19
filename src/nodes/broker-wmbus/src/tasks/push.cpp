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

namespace smf {

    push::push(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, ipt::toggle::server_vec_t&& cfg
		, ipt::push_channel&& pcc)
	: sigs_{ 
		std::bind(&push::connect, this),
		std::bind(&push::send_sml, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&push::send_mbus, this),
		std::bind(&push::send_dlms, this),
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
	{
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("connect", slot++);
            sp->set_channel_name("send.sml", slot++);
            sp->set_channel_name("send.mbus", slot++);
            sp->set_channel_name("send.dlms", slot++);
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
        CYNG_LOG_INFO(logger_, "[push] sml " << payload.size() << " bytes payload to ");

        //
        //  generate SML open response message
        //

        //
        //  send payload
        //
        // bus_.

        //
        //  generate SML close response message
        //
    }
    void push::send_mbus() {
        CYNG_LOG_INFO(logger_, "[push] mbus data");

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

        CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));
    }
    void push::ipt_stream(cyng::buffer_t &&data) { CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte"); }

    void push::auth_state(bool auth) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized - open " << pcc_.targets_.size() << " push channels");
            BOOST_ASSERT(bus_.is_authorized());
            //
            //  ToDo: open push channel
            //
            // open_push_channel();
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
        }
    }

    void push::open_push_channel() { bus_.open_channels(pcc_); }
} // namespace smf
