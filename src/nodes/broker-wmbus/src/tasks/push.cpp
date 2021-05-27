/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/push.h>
#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	push::push(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, ipt::toggle::server_vec_t&& cfg
		, ipt::push_channel&& pcc)
	: sigs_{ 
		std::bind(&push::connect, this),
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
			sp->set_channel_name("connect", 0);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
		}
	}

	push::~push()
	{
#ifdef _DEBUG_BROKER_WMBUS
		std::cout << "push(~)" << std::endl;
#endif
	}


	void push::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop push task");
		bus_.stop();
	}

	void push::connect()
	{
		//
		//	start IP-T bus
		//
		CYNG_LOG_INFO(logger_, "initialize: IP-T client");
		bus_.start();

	}

	void push::ipt_cmd(ipt::header const& h, cyng::buffer_t&& body) {

		CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));

	}
	void push::ipt_stream(cyng::buffer_t&& data) {
		CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte");

	}

	void push::auth_state(bool auth) {
		if (auth) {
			CYNG_LOG_INFO(logger_, "[ipt] authorized");
			BOOST_ASSERT(bus_.is_authorized());
			//bus_.open_channel(pcc_);
		}
		else {
			CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
		}
	}

}


