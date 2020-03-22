/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "gateway_proxy.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <smf/cluster/generator.h>
#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/parser/srv_id_parser.h>
#include <smf/sml/event.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/vm/generator.h>
#include <cyng/sys/mac.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>
//#include <cyng/vector_cast.hpp>
#include <cyng/parser/buffer_parser.h>

#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

namespace node
{
	gateway_proxy::gateway_proxy(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, std::chrono::seconds timeout
		, bool sml_log)
	: base_(*btp)
		, logger_(logger)
		, bus_(bus)
		, vm_(vm)
		, timeout_(timeout)
		, start_(std::chrono::system_clock::now())
		, parser_([this](cyng::vector_t&& prg) {

			//CYNG_LOG_DEBUG(logger_, "sml processor - "
			//	<< prg.size()
			//	<< " instructions");

			//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));

			//
			//	execute SML programm
			//
			vm_.async_run(std::move(prg));

		}, false, sml_log, false)	//	not verbose, no log instructions
		, input_queue_()
		, output_map_()
		, open_requests_{ 0 }
		, state_{ GPS::OFFLINE_ }
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running");
	}

	cyng::continuation gateway_proxy::run()
	{	
		switch (state_) {
		case GPS::OFFLINE_:
		case GPS::WAITING_:
			//
			//	waiting for an opportunity to open a connection. If session is ready
			//	is signals OK on slot 0
			//
			if (!input_queue_.empty()) {
				vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));
			}
			break;
		case GPS::CONNECTED_:
			if (!input_queue_.empty()) {
				//
				//	run input queue
				//
				run_queue();
			}
			break;
		default:
			break;
		}

		//
		//	re-start monitor
		//
		base_.suspend(timeout_);

		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 0 - ack (connected to gateway)
	cyng::continuation gateway_proxy::process()
	{
		BOOST_ASSERT_MSG(state_ == GPS::WAITING_, "wrong state");
		if (!input_queue_.empty()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> gain access and processing input queue with "
				<< input_queue_.size()
				<< " entries");

			//
			//	update state
			//
			state_ = GPS::CONNECTED_;

			//
			//	clear remaining entries
			//
			output_map_.clear();

			//
			//	run input queue and process commands
			//
			run_queue();

		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> got connected but queue is empty - disconnect");


			//
			//	reset connection state to NOT-CONNECTED == authorized
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", static_cast<std::size_t>(cyng::async::NO_TASK)));

			//
			//	update state
			//
			state_ = GPS::OFFLINE_;
		}

		//
		//	read SML data
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 1 - EOM
	cyng::continuation gateway_proxy::process(std::uint16_t crc, std::size_t midx)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> EOM #"
			<< midx);


		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 2 - receive data
	cyng::continuation gateway_proxy::process(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received "
			<< cyng::bytes_to_str(data.size()));

		//
		//	parse SML data stream
		//
		parser_.read(data.begin(), data.end());

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[3]
	cyng::continuation gateway_proxy::process(std::string trx
		, std::uint8_t group
		, cyng::buffer_t srv_id
		, cyng::buffer_t path
		, cyng::param_t values)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> get_proc_parameter "
			<< std::string(trx.begin(), trx.end()));

		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			sml::obis const root(path);

			auto const srv_str = (srv_id.empty())
				? sml::from_server_id(pos->second.get_srv())
				: sml::from_server_id(srv_id)
				;

			cyng::param_map_t params;
			params = cyng::value_cast(values.second, params);

			if (sml::OBIS_CLASS_OP_LOG_STATUS_WORD == root) {

				sml::status_word_t word;
				sml::status wrapper(word);
				wrapper.reset(cyng::value_cast<std::uint32_t>(values.second, 0u));

				params.emplace("word", cyng::make_object(sml::to_param_map(word)));
			}

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " - "
				<< sml::get_name(root)
				<< " from "
				<< srv_str
				<< "/"
				<< sml::from_server_id(pos->second.get_srv())
				<< " ["
				<< pos->second.get_msg_type()
				<< "]: "
				<< cyng::io::to_str(params));

			bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
				, pos->second.get_tag_source()
				, pos->second.get_sequence()
				, pos->second.get_key_gw()
				, pos->second.get_tag_origin()
				, pos->second.get_msg_type()
				, srv_str
				, root.to_str()
				, params));

			//
			//	remove from map
			//
			output_map_.erase(pos);

		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[4]
	cyng::continuation gateway_proxy::process(std::string trx)
	{
		//
		//	current SML request is complete
		//	sml_public_close_response
		//
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.public.close.response ["
			<< trx
			<< "] open requests: "
			<< open_requests_
			<< " , input queue size: "
			<< input_queue_.size()
			<< ", output map: "
			<< output_map_.size());

		//
		//	update request counter
		//
		--open_requests_;

		if ((open_requests_ == 0u) && input_queue_.empty()) {

			//BOOST_ASSERT_MSG(input_queue_.empty(), "input queue is not empty");
			//
			//	update task state
			//
			state_ = GPS::OFFLINE_;

			//
			//	terminate redirection
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", static_cast<std::size_t>(cyng::async::NO_TASK)));

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> cancel redirection");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[5]
	cyng::continuation gateway_proxy::process(std::string trx
		, cyng::buffer_t srv_id
		, cyng::buffer_t path
		, cyng::param_map_t params)
	{
		//
		//	There should be entries
		//
		//BOOST_ASSERT(!output_map_.empty());
		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			sml::obis const root(path);

			auto const srv_str = (srv_id.empty())
				? sml::from_server_id(pos->second.get_srv())
				: sml::from_server_id(srv_id)
				;


			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " - "
				<< sml::get_name(root)
				<< " from "
				<< srv_str
				<< "/"
				<< sml::from_server_id(pos->second.get_srv())
				<< " ["
				<< pos->second.get_msg_type()
				<< "]: "
				<< cyng::io::to_str(params));

			bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
				, pos->second.get_tag_source()
				, pos->second.get_sequence()
				, pos->second.get_key_gw()
				, pos->second.get_tag_origin()
				, pos->second.get_msg_type()
				, srv_str
				, root.to_str()
				, params));

			//
			//	remove from map
			//
			output_map_.erase(pos);

		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation gateway_proxy::process(std::string trx
		, std::string srv
		, cyng::buffer_t const& code)
	{

		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			sml::obis attention(code);

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response #"
				<< pos->second.get_sequence()
				<< " from "
				<< srv
				<< " attention code "
				<< sml::get_attention_name(attention));

			bus_->vm_.async_run(bus_res_attention_code(pos->second.get_tag_ident()
				, pos->second.get_tag_source()
				, pos->second.get_sequence()
				, pos->second.get_tag_origin()
				, srv
				, code
				, sml::get_attention_name(attention)));
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [8]
	cyng::continuation gateway_proxy::process(std::string trx
		, std::uint32_t act_time
		, std::uint32_t reg_period
		, std::uint32_t val_time
		, cyng::buffer_t path	//	OBIS
		, cyng::buffer_t srv_id
		, std::uint32_t	stat
		, cyng::param_map_t params)
	{
		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			auto const srv_str = (srv_id.empty())
				? sml::from_server_id(pos->second.get_srv())
				: sml::from_server_id(srv_id)
				;

			sml::obis const root(path);

			//
			//	update params with specific SML_GetProfileList.Res data
			//
			params.emplace("actTime", cyng::make_object(act_time));
			params.emplace("regPeriod", cyng::make_object(reg_period));
			params.emplace("valTime", cyng::make_object(val_time));
			params.emplace("status", cyng::make_object(stat));

			//
			//	provide more event type data
			//
#ifdef __DEBUG
			auto const evt = params.find(sml::OBIS_CLASS_EVENT.to_str());
			if (evt != params.end()) {
				auto const val = cyng::value_cast<std::uint32_t>(evt->second, 0u);
				params.emplace("evtType", cyng::make_object(sml::evt_get_type(val)));
				params.emplace("evtLevel", cyng::make_object(sml::evt_get_level(val)));
				params.emplace("evtSource", cyng::make_object(sml::evt_get_source(val)));
			}
#endif

			//	[2159338-2,084e0f4a,0384,084dafce,<!18446744073709551615:user-defined>,0500153B021774,00070202,%(("010000090B00":{7,0,2019-10-10 15:23:50.00000000,null}),("81040D060000":{ff,0,0,null}),("810417070000":{ff,0,0,null}),("81041A070000":{ff,0,0,null}),("81042B070000":{fe,0,0,null}),("8181000000FF":{ff,0,818100000001,null}),("8181C789E2FF":{ff,0,00800000,null}))]]
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response #"
				<< pos->second.get_sequence()
				<< " from "
				<< srv_str
				<< " status "
				<< stat);

			bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
				, pos->second.get_tag_source()
				, pos->second.get_sequence()
				, pos->second.get_key_gw()
				, pos->second.get_tag_origin()
				, pos->second.get_msg_type()
				, srv_str
				, root.to_str()
				, params));

		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation gateway_proxy::process(
		boost::uuids::uuid tag,		//	[0] ident tag (target)
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		boost::uuids::uuid origin,	//	[3] ws tag (origin)

		std::string channel,		//	[4] channel (SML message type)
		cyng::buffer_t code,		//	[5] OBIS root code
		cyng::vector_t gw,			//	[6] TGateway/TDevice PK
		cyng::tuple_t params,		//	[7] parameters

		cyng::buffer_t srv,			//	[8] server id
		std::string name,			//	[9] name
		std::string	pwd				//	[10] pwd
	)
	{
		//
		//	add new entry in work queue
		//
		input_queue_.emplace(tag, source, seq, origin, channel, code, gw, params, srv, name, pwd, input_queue_.size());

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> new input queue entry - size: "
			<< input_queue_.size());

		BOOST_ASSERT(!input_queue_.empty());

		switch (state_) {
		case GPS::OFFLINE_:

			//
			//	update state
			//
			state_ = GPS::WAITING_;

			//
			//	waiting for an opportunity to open a connection. If session is ready
			//	is signals OK on slot [0]
			vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));

			break;
		default:
			break;
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void gateway_proxy::run_queue()
	{
		//
		//	this loop is atomic
		//
		while (!input_queue_.empty()) {

			auto const server_id = input_queue_.front().get_srv();
			auto const user = input_queue_.front().get_user();
			auto const pwd = input_queue_.front().get_pwd();
			auto const source = input_queue_.front().get_tag_source();

			//
			//	generate public open request
			//
			node::sml::req_generator sml_gen(user, pwd);
			auto trx = sml_gen.public_open(get_mac(), server_id);
			boost::ignore_unused(trx);

			auto const data = input_queue_.front();
			auto const params = cyng::make_reader(data.get_params());

			//
			//	execute top queue entry
			//
			execute_cmd(sml_gen, data, params);

			//
			//	generate close request
			//
			trx = sml_gen.public_close();
			boost::ignore_unused(trx);
			cyng::buffer_t msg = sml_gen.boxing();


			//
			//	update data throughput (outgoing)
			//
			bus_->vm_.async_run(client_inc_throughput(vm_.tag()
				, source
				, msg.size()));

			//
			//	update request counter
			//
			++open_requests_;

#ifdef SMF_IO_LOG
			cyng::io::hex_dump hd;
			hd(std::cerr, msg.begin(), msg.end());
#else
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> #"
				<< open_requests_
				<< " send "
				<< cyng::bytes_to_str(msg.size())
				<< " to gateway: "
				<< cyng::io::to_hex(msg))
#endif

			//
			//	send to gateway
			//
			vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
				, cyng::generate_invoke("stream.flush") });

			//
			//	remove top queue entry
			//
			input_queue_.pop();
		}

		BOOST_ASSERT(input_queue_.empty());
	}

	void gateway_proxy::execute_cmd(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		//	[{("section":[op-log-status-word,root-visible-devices,root-active-devices,firmware,memory,root-wMBus-status,IF_wMBUS,root-ipt-state,root-ipt-param])}]
		//	[{("name":smf-form-gw-ipt-srv),("value":0500153B022980)},{("name":smf-gw-ipt-host-1),("value":waiting...)},{("name":smf-gw-ipt-local-1),("value":4)},{("name":smf-gw-ipt-remote-1),("value":3)},{("name":smf-gw-ipt-name-1),("value":waiting...)},{("name":smf-gw-ipt-pwd-1),("value":asdasd)},{("name":smf-gw-ipt-host-2),("value":waiting...)},{("name":smf-gw-ipt-local-2),("value":3)},{("name":smf-gw-ipt-remote-2),("value":3)},{("name":smf-gw-ipt-name-2),("value":holgär)},{("name":smf-gw-ipt-pwd-2),("value":asdasd)}]

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> execute channel "
			<< data.get_msg_type());
		
		switch (data.get_msg_code()) {

		case node::sml::BODY_GET_PROFILE_PACK_REQUEST:
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> not implemented yet "
				<< data.get_msg_type());
			break;
		case node::sml::BODY_GET_PROFILE_LIST_REQUEST:
			execute_cmd_get_profile_list(sml_gen, data, params);
			break;

		case node::sml::BODY_GET_PROC_PARAMETER_REQUEST:
			execute_cmd_get_proc_param(sml_gen, data, params);
			break;

		case node::sml::BODY_SET_PROC_PARAMETER_REQUEST:
			execute_cmd_set_proc_param(sml_gen, data, params);
			break;

		case node::sml::BODY_GET_LIST_REQUEST:
			execute_cmd_get_list_request(sml_gen, data, params);
			break;

		default:
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown channel "
				<< data.get_msg_type());

			break;
		}
	}

	void gateway_proxy::execute_cmd_get_profile_list(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		if (sml::OBIS_CLASS_OP_LOG == data.get_root()) {

			//
			//	get time range
			//
			auto const hours = cyng::numeric_cast<std::uint32_t>(params.get("range"), 24u);

			//
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< data.get_msg_type()
				<< " - range: "
				<< hours
				<< " hours");

			//	[
			//		cb4d6c74-be4c-481b-8317-7b0c03191ed6,
			//		9f773865-e4af-489a-8824-8f78a2311278,
			//		75,
			//		ad4d7f18-0968-4fd9-8904-dd9296cc8132,
			//		GetProfileListRequest,
			//		8181C789E1FF,
			//		[bd065994-fe7b-4985-b2ab-d9d64082ecfe],
			//		{("range":144)},
			//		0500153B02297E,
			//		operator,
			//		operator
			//	]

			push_trx(sml_gen.get_profile_list(data.get_srv()
				, std::chrono::system_clock::now() - std::chrono::hours(hours)
				, std::chrono::system_clock::now()
				, data.get_root()), data);	//	81 81 C7 89 E1 FF
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< data.get_msg_type()
				<< " - unknown path "
				<< data.get_root().to_str());
		}

	}

	void gateway_proxy::execute_cmd_get_proc_param(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		//
		//	generate get process parameter requests
		//
		node::sml::obis const root(data.get_root());
		if (sml::OBIS_ROOT_SENSOR_PARAMS == root ||
			sml::OBIS_ROOT_DATA_COLLECTOR == root ||
			sml::OBIS_PUSH_OPERATIONS == root) {

			//
			//	data mirror
			//	push operations
			//
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {
				push_trx(sml_gen.get_proc_parameter(r.first, root), data);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< data.get_msg_type()
					<< " - invalid meter id: "
					<< meter);
			}
		}
		else if (sml::OBIS_ROOT_ACCESS_RIGHTS == root) {

			auto const obj_path = cyng::to_vector(params.get("path"));
			//auto const role_nr = cyng::numeric_cast<std::uint8_t>(params.get("roleNr"), 1);		//	81 81 81 60 rr FF
			//auto const user_nr = cyng::numeric_cast<std::uint8_t>(params.get("userNr"), 1);		//	81 81 81 60 rr uu
			//auto const meter_nr = cyng::numeric_cast<std::uint8_t>(params.get("meterNr"), 1);	//	81 81 81 64 01 mm
			if (obj_path.size() == 3) {
				//auto const vec = cyng::vector_cast(obj_path, static_cast<std::uint8_t>(1));

				auto const role = cyng::numeric_cast<std::uint8_t>(obj_path.at(0), 1);
				auto const user = cyng::numeric_cast<std::uint8_t>(obj_path.at(1), 1);
				auto const meter = cyng::numeric_cast<std::uint8_t>(obj_path.at(2), 1);
				sml::obis_path path({ root
					, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, 0xFF)
					, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, user)
					, sml::make_obis(0x81, 0x81, 0x81, 0x64, 0x01, meter)
					});
				push_trx(sml_gen.get_proc_parameter(data.get_srv(), path), data);
			}
			else {
				//sml::obis_path path({ root });
				push_trx(sml_gen.get_proc_parameter(data.get_srv(), { root }), data);
			}
		}
		else {
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), root), data);
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		//
		//	generate set process parameter requests
		//

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> set "
			<< sml::from_server_id(data.get_srv())
			<< " root "
			<< data.get_root().to_str());

		if (sml::OBIS_ROOT_IPT_PARAM == data.get_root()) {

			//
			//	modify IP-T parameters
			//

			cyng::vector_t vec;
			vec = cyng::value_cast(params.get("ipt"), vec);
			execute_cmd_set_proc_param_ipt(sml_gen, data, vec);
		}
		else if (sml::OBIS_IF_wMBUS == data.get_root()) {

			//
			//	modify wireless IEC parameters
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(params.get("wmbus"), tpl);
			execute_cmd_set_proc_param_wmbus(sml_gen, data, tpl);
		}
		else if (sml::OBIS_IF_1107 == data.get_root()) {

			//
			//	modify wired IEC parameters
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(params.get("iec"), tpl);
			execute_cmd_set_proc_param_iec(sml_gen, data, tpl);
		}
		else if (sml::OBIS_REBOOT == data.get_root()) {

			//
			//	reboot the gateway
			//
			sml_gen.set_proc_parameter_restart(data.get_srv());
		}
		else if (sml::OBIS_ACTIVATE_DEVICE == data.get_root()) {

			//
			//	activate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				execute_cmd_set_proc_param_activate(sml_gen, data, nr, r.first);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
		}
		else if (sml::OBIS_DEACTIVATE_DEVICE == data.get_root()) {

			//
			//	deactivate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				execute_cmd_set_proc_param_deactivate(sml_gen, data, nr, r.first);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
		}
		else if (sml::OBIS_DELETE_DEVICE == data.get_root()) {

			//
			//	remove a meter device from list of visible meters
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				execute_cmd_set_proc_param_delete(sml_gen, data, nr, r.first);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
		}
		else if (sml::OBIS_ROOT_SENSOR_PARAMS == data.get_root()) {

			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> OBIS_ROOT_SENSOR_PARAMS not implemented yet: "
				<< sml::OBIS_ROOT_SENSOR_PARAMS.to_str());

			//
			//	change root parameters of meter
			//
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				//	data: this.tabMeter.data
                //	data: {
                //	    devClass: '-',
                //	    maker: '-',
                //	    status: 0,
                //	    bitmask: '00 00',
                //	    interval: 0,
                //	    lastRecord: "1964-04-20",
                //	    pubKey: '',
                //	    aesKey: '',
                //	    user: '',
                //	    pwd: '',
                //	    timeRef: 0
                //	}
				

				execute_cmd_set_proc_param_meter(sml_gen
					, data
					, r.first
					, cyng::value_cast<std::string>(params["data"].get("user"), "")
					, cyng::value_cast<std::string>(params["data"].get("pwd"), "")
					, cyng::value_cast<std::string>(params["data"].get("pubKey"), "")
					, cyng::value_cast<std::string>(params["data"].get("aesKey"), ""));
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< data.get_root().to_str()
					<< " invalid meter id: "
					<< meter);
			}
		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown OBIS code for SetProcParameterRequest: "
				<< data.get_root().to_str());
		}
	}

	void gateway_proxy::execute_cmd_get_list_request(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> get "
			<< sml::from_server_id(data.get_srv())
			<< " GetListRequest: "
			<< data.get_root().to_str());

		//
		//	generate get process parameter requests
		//

		if (sml::OBIS_LIST_CURRENT_DATA_RECORD == data.get_root()) {

			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {
				push_trx(sml_gen.get_list_last_data_record(get_mac().to_buffer()
					, r.first), data);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_ipt(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::vector_t vec)
	{
		std::uint8_t idx{ 1u };	//	index starts with 1
		for (auto const& p : vec) {

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(p, tpl);
			for (auto const& val : tpl) {

				//	host
				//	port
				//	user
				//	pwd
				cyng::param_t param;
				param = cyng::value_cast(val, param);

				if (boost::algorithm::equals(param.first, "host")) {

					auto address = cyng::value_cast<std::string>(param.second, "0.0.0.0");
					push_trx(sml_gen.set_proc_parameter_ipt_host(data.get_srv()
						, idx
						, address), data);
				}
				else if (boost::algorithm::equals(param.first, "port")) {
					auto port = cyng::numeric_cast<std::uint16_t>(param.second, 26862u);
					push_trx(sml_gen.set_proc_parameter_ipt_port_local(data.get_srv()
						, idx
						, port), data);
				}
				else if (boost::algorithm::equals(param.first, "user")) {
					auto str = cyng::value_cast<std::string>(param.second, "");
					push_trx(sml_gen.set_proc_parameter_ipt_user(data.get_srv()
						, idx
						, str), data);
				}
				else if (boost::algorithm::equals(param.first, "pwd")) {
					auto str = cyng::value_cast<std::string>(param.second, "");
					push_trx(sml_gen.set_proc_parameter_ipt_pwd(data.get_srv()
						, idx
						, str), data);
				}
			}

			//
			//	next IP-T server 
			//
			++idx;
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_t const& params)
	{
		for (auto const& obj : params) {

			cyng::param_t param;
			param = cyng::value_cast(obj, param);

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set w-MBus parameter "
				<< param.first
				<< ": "
				<< cyng::io::to_str(param.second));

			if (boost::algorithm::equals(param.first, "active")) {

				const auto b = cyng::value_cast(param.second, false);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_INSTALL_MODE }
					, b), data);
			}
			else if (boost::algorithm::equals(param.first, "protocol")) {

				//
				//	protocol type is encodes as a single character
				//
				auto const val = cyng::value_cast<std::string>(param.second, "S");

				push_trx(sml_gen.set_proc_parameter_wmbus_protocol(data.get_srv()
					//
					//	function to convert the character into a numeric value
					//
					, [](std::string str)->std::uint8_t {
						if (!str.empty()) {
							switch (str.at(0)) {
							case 'S':	return 1;
							case 'A':	return 2;
							case 'P':	return 3;
							default:
								break;
							}
						}
						return 0;
					}(val)), data);

			}
			else if (boost::algorithm::equals(param.first, "reboot")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_REBOOT }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "sMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_MODE_S }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "tMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_MODE_T }
					, val), data);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_iec(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::tuple_t const& params)
	{
        //	active: false,
        //	autoActivation: true,
        //	loopTime: 3600,
        //	maxDataRate: 10240,
        //	minTimeout: 200,
        //	maxTimeout: 5000,
        //	maxVar: 9,
        //	protocolMode: 'C',
        //	retries: 3,
        //	rs485: null,
        //	timeGrid: 900,
        //	timeSync: 14400,
        //	devices: []
 

		for (auto const& obj : params) {

			cyng::param_t param;
			param = cyng::value_cast(obj, param);

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set IEC parameter "
				<< param.first
				<< ": "
				<< cyng::io::to_str(param.second));

			if (boost::algorithm::equals(param.first, "active")) {

				const auto b = cyng::value_cast(param.second, false);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_ACTIVE }
					, b), data);
			}
			else if (boost::algorithm::equals(param.first, "autoActivation")) {

				const auto b = cyng::value_cast(param.second, false);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_AUTO_ACTIVATION }
					, b), data);
			}
			else if (boost::algorithm::equals(param.first, "loopTime")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 60u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_LOOP_TIME }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "maxDataRate")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_DATA_RATE }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "minTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 200u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MIN_TIMEOUT }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "maxTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 5000u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_TIMEOUT }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "maxVar")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 9u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_VARIATION }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "protocolMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 1u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_PROTOCOL_MODE }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "retries")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 3u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_RETRIES }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "rs485")) {
				//	ignore
			}
			else if (boost::algorithm::equals(param.first, "timeGrid")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 900u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_TIME_GRID }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "timeSync")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 14400u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_TIME_SYNC }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "devices")) {
				
				//
				//	ToDo:
				//
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_meter(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, cyng::buffer_t const& meter
		, std::string user
		, std::string pwd
		, std::string pubKey
		, std::string aesKey)
	{

		//
		//	parameters are "meterId" and "data" with 
		//	devClass: '-',
		//	maker: '-',
		//	status: 0,
		//	bitmask: '00 00',
		//	interval: 0,
		//	lastRecord: "1964-04-20",
		//	pubKey: '',
		//	aesKey: '',
		//	user: '',
		//	pwd: ''
		//	timeRef: 0
		//

		//
		//	meter ID is used as server ID
		//

		//
		//	modify user name
		//
		push_trx(sml_gen.set_proc_parameter(meter
			, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_NAME }
			, user), data);

		//
		//	modify user password
		//
		push_trx(sml_gen.set_proc_parameter(meter
			, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_PWD }
			, pwd), data);

		//
		//	modify public key
		//
		//auto const new_pub_key = cyng::value_cast<std::string>(dom[1]["data"].get("pubKey"), "");
		auto const r_pub_key = cyng::parse_hex_string(pubKey);
		if (r_pub_key.second) {

			push_trx(sml_gen.set_proc_parameter(meter
				, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_PUBLIC_KEY }
				, r_pub_key.first), data);
		}

		//
		//	modify AES key
		//
		//auto const new_aes_key = cyng::value_cast<std::string>(dom[1]["data"].get("aesKey"), "");
		auto const r_aes_key = cyng::parse_hex_string(aesKey);
		if (r_aes_key.second) {

			push_trx(sml_gen.set_proc_parameter(meter
				, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_AES_KEY }
				, r_aes_key.first), data);
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_activate(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, std::uint8_t nr
		, cyng::buffer_t const& meter)
	{
		push_trx(sml_gen.set_proc_parameter(data.get_srv()
			, { sml::OBIS_ACTIVATE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFB, nr), sml::OBIS_SERVER_ID }
			, meter), data);
	}

	void gateway_proxy::execute_cmd_set_proc_param_deactivate(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, std::uint8_t nr
		, cyng::buffer_t const& meter)
	{
		push_trx(sml_gen.set_proc_parameter(data.get_srv()
			, { sml::OBIS_DEACTIVATE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFC, nr), sml::OBIS_SERVER_ID }
			, meter), data);
	}

	void gateway_proxy::execute_cmd_set_proc_param_delete(sml::req_generator& sml_gen
		, ipt::proxy_data const& data
		, std::uint8_t nr
		, cyng::buffer_t const& meter)
	{
		push_trx(sml_gen.set_proc_parameter(data.get_srv()
			, { sml::OBIS_DELETE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFD, nr), sml::OBIS_SERVER_ID }
			, meter), data);
	}

	void gateway_proxy::stop(bool shutdown)
	{
		//
		//	terminate task
		//
		//	Be careful when using resources from the session like vm_, 
		//	they may already be invalid.
		//
		auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped after "
			<< cyng::to_str(uptime));
	}

	void gateway_proxy::push_trx(std::string trx, ipt::proxy_data const& data)
	{
		auto const r = output_map_.emplace(trx, data);
		if (r.second) {
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> store trx "
				<< trx
				<< " => "
				<< data.get_msg_type());
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> store trx "
				<< trx
				<< " => "
				<< data.get_msg_type()
				<< " failed");
		}
	}

	cyng::mac48 gateway_proxy::get_mac() const
	{
		auto const macs = cyng::sys::retrieve_mac48();

		return (macs.empty())
			? macs.front()
			: cyng::generate_random_mac48()
			;
	}

}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::gateway_proxy>::slot_names_({ 
			{ "ack", 0 },
			{ "shutdown", 1 }
		});

	}
}
