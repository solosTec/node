/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/filter.h>
#include <smf/mbus/flag_id.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/controller.h>

#include <fstream>
#include <iostream>

#ifdef _DEBUG_SEGW
#include <iostream>
#include <sstream>
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf
{
	filter::filter(std::weak_ptr<cyng::channel> wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& config
		, lmn_type type)
	: sigs_{
			std::bind(&filter::stop, this, std::placeholders::_1),		//	0
			std::bind(&filter::receive, this, std::placeholders::_1),	//	1
			std::bind(&filter::reset_target_channels, this, std::placeholders::_1)	//	2
		}
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, cfg_blocklist_(config, type)
		, parser_([this](mbus::radio::header const& h, cyng::buffer_t const& data) {
			auto const flag_id = h.get_manufacturer_code();
			CYNG_LOG_TRACE(logger_, "[filter] meter: " << h.get_dev_id() << " (" << mbus::decode(flag_id.first, flag_id.second) << ")");
#ifdef _DEBUG_SEGW
			//std::stringstream ss;
			//cyng::io::hex_dump<8> hd;
			//hd(ss, std::begin(data), std::end(data));
			//CYNG_LOG_DEBUG(logger_, "[" << h.get_dev_id() << "] " << data.size() << " bytes:\n" << ss.str());
#endif
			this->check(h, data);
		})
		, targets_()
	{

		//channel_.lock()->get_name()
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("receive", 1);
			sp->set_channel_name("reset-target-channels", 2);

			CYNG_LOG_INFO(logger_, "[" << cfg_blocklist_.get_task_name() << "]  ready");
		}
	}

	void filter::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "[filter] stopped");
		boost::system::error_code ec;
	}

	void filter::receive(cyng::buffer_t data) {
		CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() << "] received " << data.size() << " bytes");

#ifdef _DEBUG_SEGW
		std::stringstream ss;
		cyng::io::hex_dump<8> hd;
		hd(ss, std::begin(data), std::end(data));
		CYNG_LOG_DEBUG(logger_, "[" << cfg_blocklist_.get_task_name() << "] parse " << data.size() << " bytes:\n" << ss.str());
#endif

		if (cfg_blocklist_.is_enabled()) {

			//
			//	filter data
			//
			parser_.read(std::begin(data), std::end(data));
		}
		else {
			CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() << "] not enabled");

			//
			//	send data to broker
			//
			for (auto target : targets_) {
				target->dispatch("receive", cyng::make_tuple(data));
			}

		}
	}

	void filter::reset_target_channels(std::string name) {
		targets_ = ctl_.get_registry().lookup(name);
		CYNG_LOG_INFO(logger_, "[" << cfg_blocklist_.get_task_name() << "] has " << targets_.size() << " x target(s): " << name);
	}

	void filter::check(mbus::radio::header const& h, cyng::buffer_t const& data) {

	}

}

