/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "iec_processor.h"
#include <smf/ipt/bus.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>

#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/tuple_cast.hpp>
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

			//CYNG_LOG_DEBUG(logger_, "IEC processor "
			//	<< channel
			//	<< ':'
			//	<< source
			//	<< ':'
			//	<< target
			//	<< " - "
			//	<< prg.size()
			//	<< " instructions");

			//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
			//std::cerr << cyng::io::to_str(prg) << std::endl;

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false)	//	not verbose
		, total_bytes_(0)
		, meter_id_()
		, bcc_(false)
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
		vm_.register_function("iec.data.start", 1, std::bind(&iec_processor::iec_data_start, this, std::placeholders::_1));
		vm_.register_function("iec.data.line", 5, std::bind(&iec_processor::iec_data_line, this, std::placeholders::_1));
		vm_.register_function("iec.data.bcc", 1, std::bind(&iec_processor::iec_bcc, this, std::placeholders::_1));
		vm_.register_function("iec.data.eof", 1, std::bind(&iec_processor::iec_data_eof, this, std::placeholders::_1));

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

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
			mux_.post(tid_, 11, cyng::tuple_factory("IEC", line_, tag));

			//
			//	From here on, the behavior is undefined.
			//

		});
	}

	void iec_processor::parse(cyng::buffer_t const& data)
	{

#ifdef _DEBUG
		cyng::io::hex_dump hd;
		std::stringstream ss;
		hd(ss, data.begin(), data.end());
		CYNG_LOG_DEBUG(logger_, "iec_processor dump \n" << ss.str());
#endif

		parser_.read(data.begin(), data.end());

		total_bytes_ += data.size();

		CYNG_LOG_TRACE(logger_, cyng::bytes_to_str(total_bytes_)
			<< " bytes IEC data processed #"
			<< line_);

	}

	void iec_processor::iec_data_start(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "iec.data.start " << cyng::io::to_str(frame));
	}

	void iec_processor::iec_bcc(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] pk
			bool		//	[1] BCC
		>(frame);

		//
		//	print IEC message number
		//
		bcc_ = std::get<1>(tpl);
		if (bcc_) {
			CYNG_LOG_TRACE(logger_, "BCC OK for data set "
				<< std::get<0>(tpl));
		}
		else {
			CYNG_LOG_ERROR(logger_, "invalid BCC for IEC data set "
				<< std::get<0>(tpl));
		}
	}


	void iec_processor::iec_data_line(cyng::context& ctx)
	{
		//	[010104080362,00000.000,,0000000000000,14829735431805718452]
		//
		//	* OBIS
		//	* value
		//	* unit
		//	* status
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "iec.data.line " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] pk
			cyng::buffer_t,		//	[1] obis
			std::string,		//	[2] value
			std::string,		//	[3] unit
			std::string,		//	[4] status
			std::size_t			//	[5] counter
		>(frame);

		CYNG_LOG_TRACE(logger_, "iec.data.line #"
			<< std::get<5>(tpl)
			<< " "
			<< sml::obis(std::get<1>(tpl))
			<< " = "
			<< std::get<2>(tpl)
			<< " "
			<< std::get<3>(tpl)
			<< " "
			<< std::get<4>(tpl));

		if (sml::obis(std::get<1>(tpl)) == sml::OBIS_METER_ADDRESS) {
			meter_id_ = std::get<2>(tpl);
			CYNG_LOG_TRACE(logger_, "meter ID is " << meter_id_);
		}

		//
		//	post data to all consumers
		//
		cyng::param_map_t params;
		params["value"] = frame.at(2);
		params["unit"] = frame.at(3);
		params["status"] = frame.at(4);

		for (auto tid : consumers_) {
			mux_.post(tid, 1, cyng::tuple_factory(line_
				, std::get<0>(tpl)	//	[uuid] pk
				, std::get<1>(tpl)	//	[buffer_t] OBIS
				, std::get<5>(tpl)	//	[size_t] index
				, params
			));
		}

	}

	void iec_processor::iec_data_eof(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "iec.data.eof " 
			<< meter_id_
			<< ": "
			<< cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] pk
			std::size_t			//	[1] size
		>(frame);

		//
		//	post data to all consumers
		//
		for (auto tid : consumers_) {
			mux_.post(tid, 2, cyng::tuple_factory(line_, std::get<0>(tpl), meter_id_, bcc_, std::get<1>(tpl)));
		}

		//
		//	stop VM
		//
		stop();

	}

}
