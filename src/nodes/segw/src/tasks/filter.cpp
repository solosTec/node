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

//#ifdef _DEBUG_SEGW
#include <iostream>
#include <sstream>
#include <cyng/io/hex_dump.hpp>
//#endif

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
			std::bind(&filter::reset_target_channels, this, std::placeholders::_1),	//	2
			std::bind(&filter::update_statistics, this),	//	3
	}
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, cfg_blocklist_(config, type)
		, cfg_gpio_(config)
		, parser_([this](mbus::radio::header const& h, mbus::radio::tpl const& t, cyng::buffer_t const& data) {
			auto const flag_id = h.get_manufacturer_code();
			CYNG_LOG_TRACE(logger_, "[filter] meter: " << h.get_id() << " (" << mbus::decode(flag_id.first, flag_id.second) << ")");
#ifdef _DEBUG_SEGW
			{
				std::stringstream ss;
				cyng::io::hex_dump<8> hd;
				hd(ss, std::begin(data), std::end(data));
				CYNG_LOG_DEBUG(logger_, "[" << h.get_dev_id() << "] " << data.size() << " bytes:\n" << ss.str());
			}
#endif
			this->check(h, t, data);
		})
		, targets_()
		, access_times_()
		, accumulated_bytes_{ 0 }
	{

		//channel_.lock()->get_name()
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("receive", 1);
			sp->set_channel_name("reset-target-channels", 2);
			sp->set_channel_name("update-statistics", 3);

			CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
			CYNG_LOG_INFO(logger_, "[" << cfg_blocklist_.get_task_name() << "]  ready");

			//	stop blinking
			if (cfg_gpio_.is_enabled()) {

				auto const gpio_task_name = cfg_gpio::get_name(lmn_type::ETHERNET);
				ctl_.get_registry().dispatch(gpio_task_name, "turn", cyng::make_tuple(false));
			}
			//	update statistics
			sp->suspend(std::chrono::seconds(1), 3, cyng::make_tuple());

		}
	}

	void filter::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "[filter] stopped");
		boost::system::error_code ec;
	}

	void filter::receive(cyng::buffer_t data) {
		CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() << "] received " << data.size() << " bytes");

#ifdef _DEBUG_SEGW
		{
			std::stringstream ss;
			cyng::io::hex_dump<8> hd;
			hd(ss, std::begin(data), std::end(data));
			CYNG_LOG_DEBUG(logger_, "[" << cfg_blocklist_.get_task_name() << "] parse " << data.size() << " bytes:\n" << ss.str());
		}
#endif


		if (cfg_blocklist_.is_enabled()) {

			//
			//	filter data
			//
			parser_.read(std::begin(data), std::end(data));
		}
		else {
			if (targets_.empty())	{
				CYNG_LOG_WARNING(logger_, "[" << cfg_blocklist_.get_task_name() << "] has no targets - drop " << data.size() << " bytes");
			}
			else {
				accumulated_bytes_ += data.size();
			}
			//
			//	send data to broker
			//
			for (auto target : targets_) {
				CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() << "] pass through " << target->get_name());
				cyng::buffer_t tmp = data;	//	copy
				target->dispatch("receive", cyng::make_tuple(std::move(tmp)));
			}

		}
	}

	void filter::reset_target_channels(std::string name) {
		targets_ = ctl_.get_registry().lookup(name);
		CYNG_LOG_INFO(logger_, "[" << cfg_blocklist_.get_task_name() << "] has " << targets_.size() << " x target(s): " << name);
	}

	void filter::check(mbus::radio::header const& h, mbus::radio::tpl const& t, cyng::buffer_t const& payload) {

		auto const id = h.get_id();
		if (cfg_blocklist_.is_listed(id)) {
			if (!cfg_blocklist_.is_drop_mode()) {

				//
				// not blocked
				//	passthrough
				//
				if (check_frequency(id)) {
					accumulated_bytes_ += payload.size();

					for (auto target : targets_) {
						target->dispatch("receive", cyng::make_tuple(mbus::radio::restore_data(h, t, payload)));
					}
				}
			}
			else {
				//	drop meter
				CYNG_LOG_WARNING(logger_, "[" << cfg_blocklist_.get_task_name() << "] blocks meter " << h.get_dev_id());
			}
		}
		else {
			if (cfg_blocklist_.is_drop_mode()) {
				//
				//	accept mode and listed
				//	passthrough
				//
				accumulated_bytes_ += payload.size();
				if (check_frequency(id)) {
					for (auto target : targets_) {
						target->dispatch("receive", cyng::make_tuple(mbus::radio::restore_data(h, t, payload)));
					}
				}
			}
			else {
				//	drop meter
				CYNG_LOG_WARNING(logger_, "[" << cfg_blocklist_.get_task_name() << "] doesn't accept meter " << h.get_dev_id());
			}
		}
	}

	bool filter::check_frequency(std::string const& id) {

		//
		//	check if frequency test is enabled
		//
		if (!cfg_blocklist_.is_max_frequency_enabled())	return true;

		//
		//	search meter id
		//
		auto pos = access_times_.find(id);
		if (pos != access_times_.end()) {

			auto const delta = cfg_blocklist_.get_max_frequency();

			//
			//	calculate the elapsed time sind last access
			//
			auto const elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - pos->second);
			CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() 
				<< "] elapsed time of meter " 
				<< id 
				<< " since last access is " 
				<< elapsed.count() 
				<< " seconds"
				<< " (min = "
				<< delta.count()
				<< ")");


			if (elapsed < delta) {

				auto const remaining = std::chrono::duration_cast<std::chrono::seconds>(delta - elapsed);

				CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name()
					<< "] meter "
					<< id
					<< " has "
					<< remaining.count()
					<< " seconds to wait");
				//
				//	to fast
				//
				return false;
			}

			//
			//	update access times
			//
			pos->second = std::chrono::system_clock::now();
			return true;
		}

		access_times_.emplace(id, std::chrono::system_clock::now());
		CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() << "] add meter " << id << " to access times list (" << access_times_.size() << ")");
		return true;
		
	}

	void filter::update_statistics() {
		if (accumulated_bytes_ > 0) {
			CYNG_LOG_TRACE(logger_, "[" << cfg_blocklist_.get_task_name() << "] update statistics: " << accumulated_bytes_);
			if (accumulated_bytes_ < 128) {
				flash_led(std::chrono::milliseconds(500), 2);
			}
			else if (accumulated_bytes_ < 512) {
				flash_led(std::chrono::milliseconds(250), 4);
			}
			else {
				flash_led(std::chrono::milliseconds(125), 8);
			}
			accumulated_bytes_ = 0;
		}
		auto sp = channel_.lock();
		if (sp)	sp->suspend(std::chrono::seconds(1), 3, cyng::make_tuple());
	}

	void filter::flash_led(std::chrono::milliseconds ms, std::size_t count) {

		//
		//	LED control
		//
		if (cfg_gpio_.is_enabled()) {

			auto const gpio_task_name = cfg_gpio::get_name(lmn_type::ETHERNET);
			ctl_.get_registry().dispatch(gpio_task_name, "flashing", cyng::make_tuple(ms, count));
		}
	}
}

