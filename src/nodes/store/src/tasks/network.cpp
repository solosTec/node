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
		, boost::uuids::uuid tag
		, cyng::logger logger
		, std::string const& node_name
		, ipt::toggle::server_vec_t&& tgl
		, std::vector<std::string> const& config_types
		, std::vector<std::string> const& sml_targets
		, std::vector<std::string> const& iec_targets)
	: sigs_{ 
		std::bind(&network::connect, this, std::placeholders::_1),
		std::bind(&network::stop, this, std::placeholders::_1)
	}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, toggle_(std::move(tgl))
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
		if (bus_)	bus_->stop();
	}

	void network::connect(std::string model)
	{
		//
		//	start IP-T bus
		//
		//cfg_hardware hw_cfg(cfg_);
		CYNG_LOG_INFO(logger_, "initialize: IP-T client as " << model);
		try {
			bus_ = std::make_unique<ipt::bus>(ctl_.get_ctx()
				, logger_
				, std::move(toggle_)
				, model
				, std::bind(&network::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
				, std::bind(&network::ipt_stream, this, std::placeholders::_1));
			bus_->start();
		}
		catch (std::exception const& ex) {
			CYNG_LOG_ERROR(logger_, "start IP-T client failed " << ex.what());
			bus_.reset();
		}
	}

	void network::ipt_cmd(ipt::header const& h, cyng::buffer_t&& body) {

		CYNG_LOG_TRACE(logger_, "router ipt cmd " << ipt::command_name(h.command_));

	}
	void network::ipt_stream(cyng::buffer_t&& data) {
		CYNG_LOG_TRACE(logger_, "router ipt stream " << data.size() << " byte");

	}

}


