/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/network.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	network::network(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, boost::uuids::uuid tag)
	: sigs_{ 
		std::bind(&network::connect, this),
		std::bind(&network::stop, this, std::placeholders::_1)
	}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, bus_()
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("connect", 0);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
		}

	}

	network::~network()
	{
#ifdef _DEBUG_STORE
		std::cout << "network(~)" << std::endl;
#endif
	}


	void network::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop network task(" << tag_ << ")");
		//bus_.stop();
	}

	void network::connect()
	{
		//
		//	start IP-T bus
		//
		//cfg_hardware hw_cfg(cfg_);
		//CYNG_LOG_INFO(logger_, "initialize: IP-T client as " << hw_cfg.get_model());
		try {
			//bus_ = std::make_unique<ipt::bus>(ctl_.get_ctx()
			//	, logger_
			//	, ipt_cfg.get_toggle()
			//	, hw_cfg.get_model()
			//	, std::bind(&router::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			//	, std::bind(&router::ipt_stream, this, std::placeholders::_1));
			//bus_->start();
		}
		catch (std::exception const& ex) {
			CYNG_LOG_ERROR(logger_, "start IP-T client failed " << ex.what());
			bus_.reset();
		}

	}
}


