/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "iec_processor.h"
#include "../message_ids.h"
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
		, status_()
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
		vm_.register_function("iec.data.line", 8, std::bind(&iec_processor::iec_data_line, this, std::placeholders::_1));
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
			mux_.post(tid, CONSUMER_CREATE_LINE, cyng::tuple_factory(line_, target));
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
		vm_.halt();

		//
		//	wait for VM
		//
		const auto tag = vm_.tag();
		if (vm_.wait(12, std::chrono::milliseconds(10))) {

			CYNG_LOG_INFO(logger_, "IEC processor "
				<< line_
				<< ':'
				<< tag
				<< " stopped");
			//
			//	send shutdown message to remove this line
			//
			mux_.post(tid_, STORE_EVENT_REMOVE_PROCESSOR, cyng::tuple_factory("IEC", line_, tag));

			//
			//	From here on, the behavior is undefined.
			//

		}
		else {

			CYNG_LOG_ERROR(logger_, "IEC processor "
				<< line_
				<< ':'
				<< tag
				<< " couldn't be stopped");
		}
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
		//	* unit-code
		//	* status
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "iec.data.line " << cyng::io::to_str(frame));

		boost::uuids::uuid tag;
		sml::obis code;			//	[1] OBIS code
		std::string value;		//	[2] value
		std::int8_t scaler;		//	[3] scaler
		std::string unit;		//	[4] unit
		std::uint8_t unit_code;	//	[5] unit
		std::string status;		//	[6] status
		std::uint64_t idx;		//	[7] index

		std::tie(tag, code, value, scaler, unit, unit_code, status, idx) = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] record tag
			cyng::buffer_t,			//	[1] OBIS code
			std::string,			//	[2] value
			std::int8_t,			//	[3] scaler
			std::string,			//	[4] unit
			std::uint8_t,			//	[5] unit-code
			std::string,			//	[6] status
			std::uint64_t			//	[7] index
		>(frame);

		//auto const tpl = cyng::tuple_cast<
		//	boost::uuids::uuid,	//	[0] pk
		//	cyng::buffer_t,		//	[1] obis
		//	std::string,		//	[2] value
		//	std::string,		//	[3] unit
		//	std::uint8_t,		//	[4] unit-code
		//	std::string,		//	[5] status
		//	std::size_t			//	[6] counter
		//>(frame);

		CYNG_LOG_TRACE(logger_, "iec.data.line #"
			<< status
			<< " "
			<< code
			<< " = "
			<< value
			<< " "
			<< unit
			<< " "
			<< status);

		if (code == sml::OBIS_METER_ADDRESS) {
			meter_id_ = status;
			CYNG_LOG_TRACE(logger_, "meter ID is " << meter_id_);
		}
		else if (code == sml::OBIS_MBUS_STATE) {
			//0-0:97.97.0*255
			status_ = status;
			CYNG_LOG_TRACE(logger_, "status is " << status_);
		}

		//
		//	post data to all consumers
		//
		cyng::param_map_t params;
		params["value"] = frame.at(2);
		params["unit"] = frame.at(4);
		params["status"] = frame.at(6);

		for (auto tid : consumers_) {
			mux_.post(tid, CONSUMER_PUSH_DATA, cyng::tuple_factory(line_
				, tag	//	[uuid] pk
				, code.to_buffer()	//	[buffer_t] OBIS
				, idx	//	[size_t] index
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
		//	close all consumers
		//
		for (auto tid : consumers_) {
			mux_.post(tid, CONSUMER_REMOVE_LINE, cyng::tuple_factory(line_, std::get<0>(tpl), meter_id_, status_, bcc_, std::get<1>(tpl)));
		}

		//
		//	stop VM
		//
		stop();

	}

}
