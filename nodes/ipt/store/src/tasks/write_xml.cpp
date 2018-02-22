/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "write_xml.h"
#include <cyng/async/task/task_builder.hpp>
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

	write_xml::write_xml(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::string root_dir
		, std::string root_name
		, std::string endocing
		, std::uint32_t channel
		, std::uint32_t source
		, std::string target)
	: base_(*btp)
		, logger_(logger)
		, root_dir_(root_dir)
		, vm_(base_.mux_.get_io_service(), tag)
		, parser_([this](cyng::vector_t&& prg) {

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> process "
				<< prg.size()
				<< " instructions");

			//
			//	execute programm
			//
			vm_.run(std::move(prg));

		}, false)
		, exporter_(endocing, root_name, channel, source, target)
		, initialized_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< channel
			<< ':'
			<< source
			<< " established ");

		cyng::register_logger(logger_, vm_);
		vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		vm_.run(cyng::register_function("sml.msg", 1, std::bind(&write_xml::sml_msg, this, std::placeholders::_1)));
		vm_.run(cyng::register_function("sml.eom", 1, std::bind(&write_xml::sml_eom, this, std::placeholders::_1)));

	}

	void write_xml::run()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running");

		if (initialized_)
		{
			initialized_ = true;
		}
		else
		{
			//
			//	timeout
			//
			base_.suspend(std::chrono::seconds(30));
		}
	}

	void write_xml::stop()
	{
		//
		//	halt VM
		//
		vm_.halt();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");

	}

	cyng::continuation write_xml::process(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> processing "
			<< data.size()
			<< " bytes");

		//cyng::io::hex_dump hd;
		//std::stringstream ss;
		//hd(ss, data.begin(), data.end());
		//CYNG_LOG_TRACE(logger_, "write_xml dump \n" << ss.str());


		parser_.read(data.begin(), data.end());
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation write_xml::process()
	{
		return cyng::continuation::TASK_STOP;
	}

	void write_xml::sml_msg(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.msg #"
			<< idx);

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		exporter_.read(msg, idx);
	}


	void write_xml::sml_eom(cyng::context& ctx)
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

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.eom #" 
			<< idx
			<< ' '
			<< p);

		boost::filesystem::remove(p);
		exporter_.write(p);
		exporter_.reset();

		ctx.attach(cyng::generate_invoke("stop.writer", base_.get_id()));
	}

}
