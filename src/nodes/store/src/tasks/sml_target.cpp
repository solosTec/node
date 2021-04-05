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
	{
		//CYNG_LOG_WARNING(logger_, "stop network task(" << tag_ << ")");
		//if (bus_)	bus_->stop();
	}

	void sml_target::register_target(std::string name) {
		bus_.register_target(name, std::bind(&sml_target::receive, this, std::placeholders::_1));
	}

	void sml_target::receive(cyng::buffer_t&& data) {

		CYNG_LOG_TRACE(logger_, "[sml] receive " << data.size() << " bytes");
	}

	//void sml_target::ipt_cmd(ipt::header const& h, cyng::buffer_t&& body) {

	//	CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));

	//}
	//void sml_target::ipt_stream(cyng::buffer_t&& data) {
	//	CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte");

	//}

	//void sml_target::auth_state(bool auth) {
	//	if (auth) {
	//		CYNG_LOG_INFO(logger_, "[ipt] authorized");
	//		register_targets();
	//	}
	//	else {
	//		CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
	//	}
	//}

	//void sml_target::register_targets() {
	//	for (auto const& sml_target : sml_targets_) {
	//		CYNG_LOG_TRACE(logger_, "[ipt] register sml sml_target " << sml_target);

	//	}

	//	for (auto const& sml_target : iec_targets_) {
	//		CYNG_LOG_TRACE(logger_, "[ipt] register iec sml_target " << sml_target);

	//	}
	//}


}


