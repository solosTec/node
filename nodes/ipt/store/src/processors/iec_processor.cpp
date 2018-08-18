/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "iec_processor.h"
#include <smf/ipt/bus.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/io_bytes.hpp>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{

	iec_processor::iec_processor(cyng::async::mux& m
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

			CYNG_LOG_DEBUG(logger_, "IEC processor "
				<< channel
				<< ':'
				<< source
				<< ':'
				<< target
				<< " - "
				<< prg.size()
				<< " instructions");

			CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
			std::cerr << cyng::io::to_str(prg) << std::endl;

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, true)	//	not verbose
		, total_bytes_(0)
	{
		init(channel, source, target);
	}

	void iec_processor::init(std::uint32_t channel
		, std::uint32_t source
		, std::string target)
	{
		//
		//	vm_.run() could assert since there is a small chance that the VM controller detects
		//	running_in_this_thread()
		//
		//vm_.register_function("sml.msg", 1, std::bind(&iec_processor::sml_msg, this, std::placeholders::_1));
		//vm_.register_function("sml.eom", 1, std::bind(&iec_processor::sml_eom, this, std::placeholders::_1));

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

	iec_processor::~iec_processor()
	{
		CYNG_LOG_TRACE(logger_, "iec_processor(~"
			<< line_
			<< ")");
	}

	void iec_processor::stop()
	{
		//
		//	halt VM
		//
		vm_.access([this](cyng::vm& vm) {

			//
			//	halt VM
			//
			//
			//	halt VM
			//
			const auto tag = vm_.tag();
			vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });

			CYNG_LOG_INFO(logger_, "IEC processor "
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

	void iec_processor::parse(cyng::buffer_t const& data)
	{
		//CYNG_LOG_INFO(logger_, "IEC processor processing "
		//	<< data.size()
		//	<< " bytes");

		total_bytes_ += data.size();

		CYNG_LOG_TRACE(logger_, cyng::bytes_to_str(total_bytes_)
			<< " IEC data processed");

		cyng::io::hex_dump hd;
		std::stringstream ss;
		hd(ss, data.begin(), data.end());
		CYNG_LOG_TRACE(logger_, "iec_processor dump \n" << ss.str());


		parser_.read(data.begin(), data.end());
	}

	void iec_processor::sml_msg(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_INFO(logger_, "IEC processor sml.msg #"
			<< idx);

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		//exporter_.read(msg, idx);
	}


	void iec_processor::sml_eom(cyng::context& ctx)
	{
		//	[5213,3]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "sml.eom " << cyng::io::to_str(frame));

		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		//const auto p = root_dir_ / exporter_.get_filename();

		//CYNG_LOG_INFO(logger_, "IEC processor sml.eom #"
		//	<< idx
		//	<< ' '
		//	<< p);

		//boost::filesystem::remove(p);
		//exporter_.write(p);
		//exporter_.reset();

		//ctx.attach(cyng::generate_invoke("stop.writer", cyng::code::IDENT));
	}
}
