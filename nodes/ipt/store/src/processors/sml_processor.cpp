/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_processor.h"
#include "../message_ids.h"
#include <smf/ipt/bus.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/io_bytes.hpp>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <iterator>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{

	sml_processor::sml_processor(cyng::async::mux& m
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::uint32_t channel
		, std::uint32_t source
		, std::string target
		, std::size_t tid
		, std::vector<std::size_t> const& consumers)
	: logger_(logger)
		, vm_(m.get_io_service(), tag)
		, mux_(m)
		, tid_(tid)
		, consumers_(consumers)
		, line_(ipt::build_line(channel, source))
		, parser_([this, channel, source, target](cyng::vector_t&& prg) {

			//CYNG_LOG_DEBUG(logger_, "sml processor "
			//	<< channel
			//	<< ':'
			//	<< source
			//	<< ':'
			//	<< target
			//	<< " - "
			//	<< prg.size()
			//	<< " instructions");

			//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false, false, false)	//	not verbose, no log instructions
		, shutdown_(false)
		, last_activity_(std::chrono::system_clock::now())
	{
		init(channel, source, target);
	}

	void sml_processor::init(std::uint32_t channel
		, std::uint32_t source
		, std::string target)
	{
		//
		//	vm_.run() could assert since there is a small chance that the VM controller detects
		//	running_in_this_thread()
		//
		vm_.register_function("sml.msg", 3, std::bind(&sml_processor::sml_msg, this, std::placeholders::_1));
		vm_.register_function("sml.eom", 2, std::bind(&sml_processor::sml_eom, this, std::placeholders::_1));
		vm_.register_function("sml.log", 1, std::bind(&sml_processor::sml_log, this, std::placeholders::_1));

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);

		//
		//	statistics
		//
		vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), " callbacks registered"));

		//
		//	Initial message to create a new line.
		//	Consumers manage there own list of lines.
		//
		for (auto tid : consumers_) {
			mux_.post(tid, CONSUMER_CREATE_LINE, cyng::tuple_factory(line_, target));
		}
	}

	sml_processor::~sml_processor()
	{
		CYNG_LOG_TRACE(logger_, "sml_processor(~" 
			<< line_
			<< ")");
	}

	void sml_processor::stop()
	{
		//
		//	halt VM
		//
		vm_.halt();

		//
		//	wait for VM
		//
		const auto tag = vm_.tag();
		if (vm_.wait(12, std::chrono::milliseconds(10))) {

			CYNG_LOG_INFO(logger_, "SML processor "
				<< line_
				<< ':'
				<< tag
				<< " stopped");

			//
			//	send shutdown message to remove this line
			//
			mux_.post(tid_, STORE_EVENT_REMOVE_PROCESSOR, cyng::tuple_factory("SML", line_, tag));

			//
			//	From here on, the behavior is undefined.
			//

		}
		else {

			CYNG_LOG_ERROR(logger_, "SML processor "
				<< line_
				<< ':'
				<< tag
				<< " couldn't be stopped");
		}
	}


	void sml_processor::parse(cyng::buffer_t const& data)
	{
		parser_.read(data.begin(), data.end());

		CYNG_LOG_TRACE(logger_, cyng::bytes_to_str(data.size())
			<< " bytes SML data processed #"
			<< line_);

	}

	void sml_processor::sml_msg(cyng::context& ctx)
	{
		//
		//	1. tuple containing the SML data tree
		//	2. message index
		//

		const cyng::vector_t frame = ctx.get_frame();

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_INFO(logger_, "SML processor sml.msg #"
			<< idx);

		//
		//	get message body
		//
		cyng::tuple_t const msg = cyng::to_tuple(frame.at(0));

		//
		//	get SML message type
		//
		auto code = get_msg_type(msg);
		shutdown_ = sml::message_e::CLOSE_RESPONSE == code;

		if (shutdown_) {

			CYNG_LOG_INFO(logger_, "post "
				<< sml::messages::name(code)
				<< " to "
				<< consumers_.size()
				<< " consumer(s) - shutdown mode");
		}
		else {
			CYNG_LOG_TRACE(logger_, "post "
				<< sml::messages::name(code)
				<< " to "
				<< consumers_.size()
				<< " consumer(s)");
		}

		//
		//	post data to all consumers
		//
		for (auto tid : consumers_) {
			mux_.post(tid, CONSUMER_PUSH_DATA, cyng::tuple_factory(line_, code, msg));
		}

		//
		//	update activity marker
		//
		last_activity_ = std::chrono::system_clock::now();
	}


	void sml_processor::sml_eom(cyng::context& ctx)
	{
		//	[5213,3]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "sml.eom " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			std::uint16_t,	//	[0] CRC
			std::size_t		//	[1] message index
		>(frame);

		//
		//	signal end of push data
		//
		if (shutdown_) {
			for (auto tid : consumers_) {
				mux_.post(tid, CONSUMER_EOM, cyng::tuple_factory(line_, std::get<1>(tpl), std::get<0>(tpl)));
			}
		}
	}

	void sml_processor::sml_log(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "sml.log " << cyng::io::to_str(frame));
	}

	void sml_processor::test_activity()
	{
		auto delta = std::chrono::system_clock::now() - last_activity_;	
		if (shutdown_ || (delta > std::chrono::minutes(1))) {

			CYNG_LOG_INFO(logger_, "inactivity detected "
				<< cyng::to_str(delta)
				<< " - disconnect " 
				<< consumers_.size()
				<< " data consumers");

			//
			//	signal end of push data
			//
			for (auto tid : consumers_) {
				mux_.post(tid, CONSUMER_REMOVE_LINE, cyng::tuple_factory(line_));
			}
				
			//	stop VM
			stop();
		}
	}

	sml::message_e get_msg_type(cyng::tuple_t const& tpl)
	{
		if (tpl.size() == 5)
		{
			auto pos = tpl.begin();
			std::advance(pos, 3);

			//
			//	(4/5) CHOICE - msg type
			//
			cyng::tuple_t choice;
			choice = cyng::value_cast(*pos++, choice);
			BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			if (choice.size() == 2)
			{
				return static_cast<sml::message_e>(cyng::value_cast<std::uint16_t>(choice.front(), 0x0000FF02));
			}
		}

		return sml::message_e::UNKNOWN;
	}


}
