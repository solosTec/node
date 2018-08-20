/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_processor.h"
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

			CYNG_LOG_DEBUG(logger_, "sml processor "
				<< channel
				<< ':'
				<< source
				<< ':'
				<< target
				<< " - "
				<< prg.size()
				<< " instructions");

			CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false, false)	//	not verbose, no log instructions
		, shutdown_(false)
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
		vm_.register_function("sml.msg", 2, std::bind(&sml_processor::sml_msg, this, std::placeholders::_1));
		vm_.register_function("sml.eom", 2, std::bind(&sml_processor::sml_eom, this, std::placeholders::_1));

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);

		//
		//	initial message to create a new line
		//
		for (auto tid : consumers_) {
			mux_.post(tid, 0, cyng::tuple_factory(line_, target));
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
		vm_.access([this](cyng::vm& vm) {

			//
			//	halt VM
			//
			const auto tag = vm_.tag();
			vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });

			CYNG_LOG_INFO(logger_, "SML processor "
				<< line_
				<< ':'
				<< vm_.tag()
				<< " stopped");

			//
			//	send shutdown message to remove this line
			//
			mux_.post(tid_, 11, cyng::tuple_factory("SML", line_, tag));

			//
			//	From here on, the behavior is undefined.
			//
		});
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
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		//
		//	get SML message type
		//
		auto code = get_msg_type(msg);
		shutdown_ = sml::BODY_CLOSE_RESPONSE == code;

		//CYNG_LOG_TRACE(logger_, "SML message " 
		//	<< sml::messages::name(code)
		//	<< ": "
		//	<< cyng::io::to_str(msg));

		CYNG_LOG_INFO(logger_, "post "
			<< sml::messages::name(code)
			<< " to "
			<< consumers_.size()
			<< " consumer(s)");

		//
		//	post data to all consumers
		//
		for (auto tid : consumers_) {
			mux_.post(tid, 1, cyng::tuple_factory(line_, code, idx, msg));
		}
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
				mux_.post(tid, 2, cyng::tuple_factory(line_, std::get<1>(tpl), std::get<0>(tpl)));
			}

			//
			//	stop VM
			//
			stop();
		}
	}

	std::uint16_t get_msg_type(cyng::tuple_t const& tpl)
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
				return cyng::value_cast<std::uint16_t>(choice.front(), sml::BODY_UNKNOWN);
			}
		}

		return sml::BODY_UNKNOWN;
	}

}
