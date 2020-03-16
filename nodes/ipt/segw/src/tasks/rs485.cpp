/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "rs485.h"
#include "../cache.h"
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>

#include <cyng/intrinsics/buffer.h>

namespace node
{
	rs485::rs485(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cache& cfg
		, std::size_t tsk)
	: base_(*btp) 
		, logger_(logger)
		, cache_(cfg)
		, cfg_(cfg)
		, tsk_(tsk)
		, state_(cfg_.get_search_device() ? state::SEARCH : state::READOUT)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> => #"
			<< tsk_);
	}

	cyng::continuation rs485::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif

		switch (state_) {
		case state::SEARCH:
			search();
			break;
		case state::READOUT:
			readout();
			base_.suspend(cfg_.get_readout_interval());
			break;
		default:
			break;
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void rs485::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation rs485::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void rs485::search()
	{
		//	Remove the secondary address matching symbol of all the meters on BUS.
		//	10 40 FD 3D 16
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x10, 0x40, 0xFF, 0x3F, 0x16 })));

		//	initialize all meters on the bus line by using FF as broadcast address
		//	10 40 FF 3F 16
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x68, 0x0B, 0x0B, 0x68, 0x73, 0xFD, 0x52, 0x16 })));

		//	10 40 01 41 16
		auto data = cyng::make_buffer({ 0x10, 0x00, 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0x16 });
		//10 7B 01 7C 16
		//auto data = cyng::make_buffer({ 0x10, 0x7B, 0x01, 0x7C, 0x16 });
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(data));

		//	read out of Energy information from address 01
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x10, 0x7B, 0xFD, 0x78, 0x16 })));
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x10, 0x7B, 0xFD, 0x78, 0x16 })));
	}

	void rs485::readout()
	{}

}

