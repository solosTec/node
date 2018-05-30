/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "xml_processor.h"
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

	xml_processor::xml_processor(cyng::io_service_t& ios
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::string root_dir
		, std::string root_name
		, std::string endocing
		, std::uint32_t channel
		, std::uint32_t source
		, std::string target)
	: logger_(logger)
		, root_dir_(root_dir)
		, vm_(ios, tag)
		, parser_([this, channel, source, target](cyng::vector_t&& prg) {

			CYNG_LOG_DEBUG(logger_, "xml processor "
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

		}, false)
		, exporter_(endocing, root_name, channel, source, target)
	{
		init();
	}

	void xml_processor::init()
	{
		//
		//	vm_.run() could assert since there is a small chance that the VM controller detects
		//	running_in_this_thread()
		//
		if (vm_.same_thread())
		{
			vm_.register_function("sml.msg", 1, std::bind(&xml_processor::sml_msg, this, std::placeholders::_1));
			vm_.register_function("sml.eom", 1, std::bind(&xml_processor::sml_eom, this, std::placeholders::_1));
			vm_.register_function("sml.parse", 1, std::bind(&xml_processor::sml_parse, this, std::placeholders::_1));
		}
		else
		{
			vm_.register_function("sml.msg", 1, std::bind(&xml_processor::sml_msg, this, std::placeholders::_1));
			vm_.register_function("sml.eom", 1, std::bind(&xml_processor::sml_eom, this, std::placeholders::_1));
			vm_.register_function("sml.parse", 1, std::bind(&xml_processor::sml_parse, this, std::placeholders::_1));
		}

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
	}

	xml_processor::~xml_processor()
	{
		stop();
	}

	void xml_processor::stop()
	{
		//
		//	halt VM
		//
		vm_.access([this](cyng::vm& vm) {

			//
			//	halt VM
			//
			vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });
			CYNG_LOG_INFO(logger_, "XML processor stopped");
		});

	}

	void xml_processor::parse(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "XML processor processing "
			<< data.size()
			<< " bytes");

		//cyng::io::hex_dump hd;
		//std::stringstream ss;
		//hd(ss, data.begin(), data.end());
		//CYNG_LOG_TRACE(logger_, "xml_processor dump \n" << ss.str());


		parser_.read(data.begin(), data.end());
	}

	void xml_processor::sml_parse(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		cyng::buffer_t data;
		data = cyng::value_cast(frame.at(0), data);
		parse(data);
	}

	void xml_processor::sml_msg(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_INFO(logger_, "XML processor sml.msg #"
			<< idx);

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		exporter_.read(msg, idx);
	}


	void xml_processor::sml_eom(cyng::context& ctx)
	{
		//	[5213,3]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "sml.eom " << cyng::io::to_str(frame));

		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		const auto p = root_dir_ / exporter_.get_filename();

		CYNG_LOG_INFO(logger_, "XML processor sml.eom #"
			<< idx
			<< ' '
			<< p);

		boost::filesystem::remove(p);
		exporter_.write(p);
		exporter_.reset();

		ctx.attach(cyng::generate_invoke("stop.writer", cyng::code::IDENT));
	}
}
