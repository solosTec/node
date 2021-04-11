/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_target.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	sml_target::sml_target(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, ipt::bus& bus)
	: sigs_{ 
		std::bind(&sml_target::register_target, this, std::placeholders::_1),
		std::bind(&sml_target::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&sml_target::stop, this, std::placeholders::_1)
	}
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, bus_(bus)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("register", 0);
			sp->set_channel_name("receive", 1);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
		}

	}

	sml_target::~sml_target()
	{
#ifdef _DEBUG_STORE
		std::cout << "sml_target(~)" << std::endl;
#endif
	}


	void sml_target::stop(cyng::eod)
	{}

	void sml_target::register_target(std::string name) {
		bus_.register_target(name, channel_);
	}

	void sml_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t&& data) {

		CYNG_LOG_TRACE(logger_, "[sml] " << channel_.lock()->get_name() << " receive " << data.size() << " bytes");

	}

}


