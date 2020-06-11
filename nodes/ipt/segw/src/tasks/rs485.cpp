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
		, cyng::controller& vm
		, cache& cfg
		, std::size_t tsk)
	: base_(*btp)
		, logger_(logger)
		, cache_(cfg)
		, cfg_(cfg)
		, decoder_mbus_(logger, cfg, vm)
		, state_(cfg_.get_search_device() ? state::REMOVE_SECONDARY_ADDRESS : state::READOUT)
		, tsk_(tsk)
		, search_idx_(0)
		, response_(false)
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
			<< "> "
			<< get_state());
#endif

		switch (state_) {
		case state::REMOVE_SECONDARY_ADDRESS:
			remove_secondary_address();
			state_ = state::INITIALIZE_ALL_METERS;
			base_.suspend(std::chrono::seconds(4));
			break;
		case state::INITIALIZE_ALL_METERS:
			initialize_all_meters();
			state_ = state::SEARCH;
			base_.suspend(std::chrono::seconds(4));
			response_ = false;
			break;
		case state::SEARCH:
			//if (response_) {
			//	search_idx_ = 0;
			//	state_ = state::READOUT;
			//	break;
			//}
			search();
			//	slave has to wait between 11 bit times and (330 bit times + 50ms) before answering 
			base_.suspend(std::chrono::seconds(12));
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
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> ACK");

		switch (state_) {
		case state::SEARCH:
			response_ = true;
			state_ = state::QUERY;
			base_.mux_.post(tsk_, 0, cyng::tuple_factory(mbus_read_data_by_primary_address(true, search_idx_)));
			break;
		default:
			//	ignore ACK
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> ignore ACK");
			break;
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [1] - short frame
	cyng::continuation rs485::process(
		boost::uuids::uuid,	//	[0] tag
		bool ok,			//	[1] OK
		std::uint8_t c,		//	[2] C-field
		std::uint8_t a,		//	[3] A-field
		std::uint8_t cs,	//	[5] checksum
		cyng::buffer_t payload	//	[6] payload
	)
	{
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> short frame");

		if (ok) {
			decoder_mbus_.read_short_frame(c, a, cs, payload);
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [2] - long frame
	cyng::continuation rs485::process(
		boost::uuids::uuid,	//	[0] tag
		bool ok,			//	[1] OK
		std::uint8_t c,		//	[2] C-field
		std::uint8_t a,		//	[3] A-field
		std::uint8_t ci,	//	[4] CI-field
		std::uint8_t cs,	//	[5] checksum
		cyng::buffer_t payload)
	{
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> long frame");

		//if (ok) {
			decoder_mbus_.read_long_frame(c, a, ci, cs, payload);
		//}

		switch (state_) {
		case state::QUERY:
			//response_ = true;
			state_ = state::SEARCH;
			//base_.mux_.post(tsk_, 0, cyng::tuple_factory(mbus_read_data_by_primary_address(true, search_idx_)));
			search();
			break;
		default:
			//	ignore ACK
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> ignore long frame");
			break;
		}


		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [3] - ctrl frame
	cyng::continuation rs485::process(
		boost::uuids::uuid,	//	[0] tag
		bool ok,			//	[1] OK
		std::uint8_t c,		//	[2] C-field
		std::uint8_t a,		//	[3] A-field
		std::uint8_t ci,	//	[4] CI-field
		std::uint8_t cs		//	[5] checksum
	)
	{
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> ctrl frame");

		if (ok) {
			decoder_mbus_.read_ctrl_frame(c, a, ci, cs);
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void rs485::search()
	{

		//
		//
		//
		++search_idx_;
		auto const cmd = mbus_initialize_slave(search_idx_);
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cmd));

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> initialize slave "
			<< +search_idx_);

		if (search_idx_ == 0xFA) {
			search_idx_ = 0;
			state_ = state::READOUT;

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> slave primary search finished");
		}


		//	Remove the secondary address matching symbol of all the meters on BUS.
		//	10 40 FD 3D 16
		//base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x10, 0x40, 0xFF, 0x3F, 0x16 })));

		//	initialize all meters on the bus line by using FF as broadcast address
		//	10 40 FF 3F 16
		//base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x68, 0x0B, 0x0B, 0x68, 0x73, 0xFD, 0x52, 0x16 })));

		//	10 40 01 41 16
		//auto data = cyng::make_buffer({ 0x10, 0x00, 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6, 0x16 });
		//10 7B 01 7C 16
		//auto data = cyng::make_buffer({ 0x10, 0x7B, 0x01, 0x7C, 0x16 });
		//base_.mux_.post(tsk_, 0, cyng::tuple_factory(data));

		//	read out of Energy information from address 01
		//base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x10, 0x7B, 0xFD, 0x78, 0x16 })));
		//base_.mux_.post(tsk_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x10, 0x7B, 0xFD, 0x78, 0x16 })));
	}

	void rs485::remove_secondary_address()
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> initialize slave");

		auto const cmd = mbus_initialize_slave(0xFD);
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cmd));
	}
	void rs485::initialize_all_meters()
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> initialize all meters");

		auto const cmd = mbus_initialize_slave(0xFF);
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cmd));
	}

	void rs485::readout()
	{
		//	read address 9
		auto cmd = cyng::make_buffer({ 0x10, 0x7B, 0x09, 0x84, 0x16 });
		base_.mux_.post(tsk_, 0, cyng::tuple_factory(cmd));
	}


	std::string rs485::get_state() const
	{
		switch(state_) {
		case state::REMOVE_SECONDARY_ADDRESS:	return "REMOVE_SECONDARY_ADDRESS";
		case state::INITIALIZE_ALL_METERS:	return "INITIALIZE_ALL_METERS";
		case state::SEARCH:	return "SEARCH";
		case state::QUERY:	return "QUERY";
		case state::READOUT:	return "READOUT";
		}
		return "";
	}

	cyng::buffer_t mbus_initialize_slave(std::uint8_t address)
	{
		auto cmd = cyng::make_buffer({ 0x10, 0x40, address, 0x0, 0x16 });
		mbus_calculate_checksum(cmd);
		return cmd;
	}

	cyng::buffer_t mbus_read_data_by_primary_address(bool fcb, std::uint8_t address)
	{
		//	10 7B AD CS 16 (example: 10 7B 01 7C 16)
		std::uint8_t const cfield = fcb ? 0x7B : 0x5B;
		auto cmd = cyng::make_buffer({ 0x10, cfield, address, 0x00, 0x16 });
		mbus_calculate_checksum(cmd);
		return cmd;
	}

	cyng::buffer_t mbus_select_slave_by_id(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
	{
		auto cmd = cyng::make_buffer({ 0x68, 0x0B, 0x0B, 0x68, 0x73, 0xFD, 0x52, a, b, c, d, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x16 });
		mbus_calculate_checksum(cmd);
		return cmd;
	}

	cyng::buffer_t mbus_set_address(std::uint8_t a)
	{
		auto cmd = cyng::make_buffer({ 0x68, 0x06, 0x06, 0x68, 0x73, 0xFD, 0x51, 0xF1, 0x7A, a, 0x00, 0x16 });
		mbus_calculate_checksum(cmd);
		return cmd;
	}

	void mbus_calculate_checksum(cyng::buffer_t& cmd)
	{
		auto const size = cmd.size();
		BOOST_ASSERT(size > 4);
		if (size < 5)	return;
		BOOST_ASSERT(cmd.at(size - 1) == 0x16);
		if (cmd.at(size - 1) != 0x16)	return;	//	not a valid mbus command

		//	initialize checksum
		cmd.at(size - 2) = cmd.at(1);
		for (std::size_t idx = 2; idx < size - 2; ++idx) {
			//	calculate checksum
			cmd.at(size - 2) += cmd.at(idx);
		}
	}

}

