/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_processor.h"
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	sml_processor::sml_processor(cyng::io_service_t& ios
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::uint32_t channel
		, std::uint32_t source
		, std::string target
		, std::size_t tid)
	: logger_(logger)
		, vm_(ios, tag)
		, tid_(tid)
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

			//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
			//std::cerr << cyng::io::to_str(prg) << std::endl;

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false, false)	//	not verbose, no log instructions
	{
		init();
	}

	void sml_processor::init()
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
	}

	sml_processor::~sml_processor()
	{
		CYNG_LOG_TRACE(logger_, "~sml_processor()");
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
			vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });
			CYNG_LOG_INFO(logger_, "SML processor stopped");
		});
	}


	void sml_processor::parse(cyng::buffer_t const& data)
	{
		parser_.read(data.begin(), data.end());
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

		//exporter_.read(msg, idx);
		//tid_
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

		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		//const auto p = root_dir_ / exporter_.get_filename();

		//CYNG_LOG_INFO(logger_, "SML processor sml.eom #"
		//	<< idx
		//	<< ' '
		//	<< p);

		//boost::filesystem::remove(p);
		//exporter_.write(p);
		//exporter_.reset();

		//ctx.attach(cyng::generate_invoke("stop.writer", cyng::code::IDENT));
	}
}
