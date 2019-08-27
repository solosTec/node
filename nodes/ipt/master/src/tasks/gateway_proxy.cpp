/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "gateway_proxy.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/reader.h>
#include <smf/cluster/generator.h>
#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/parser/srv_id_parser.h>

#include <cyng/vm/generator.h>
#include <cyng/sys/mac.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/dom/reader.h>
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
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false, sml_log)	//	not verbose, no log instructions
		, queue_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running");
	}

	cyng::continuation gateway_proxy::run()
	{	
		if (!queue_.empty() && queue_.front().is_waiting()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> queue size "
				<< queue_.size());

			//
			//	waiting for an opportunity to open a connection. If session is ready
			//	is signals OK on slot 0
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));
		}

		//
		//	re-start monitor
		//
		base_.suspend(timeout_);

		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 0 - ack
	cyng::continuation gateway_proxy::process()
	{
		if (!queue_.empty()) {

			//
			//	update data state
			//
			queue_.front().next();

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> start processing ");


			//
			//	process commands
			//
			execute_cmd();

		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> got acknowledge but queue is empty");

			//
			//	ToDo: reset connection state to NOT-CONNECTED == authorized
			//
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
			<< "> "
			//<< sml::from_server_id(server_id_)
			<< " EOM #"
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
	cyng::continuation gateway_proxy::process(cyng::buffer_t trx, std::uint8_t, std::uint8_t, cyng::tuple_t msg, std::uint16_t crc)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.msg "
			<< std::string(trx.begin(), trx.end()));

		sml::reader reader;
		reader.set_trx(trx);
		vm_.async_run(reader.read_choice(msg));

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[4]
	cyng::continuation gateway_proxy::process(boost::uuids::uuid pk, cyng::buffer_t trx, std::size_t)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.public.close.response "
			<< std::string(trx.begin(), trx.end()));

		//
		//	current SML request is complete
		//
		if (!queue_.empty()) {
			queue_.pop();
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[5]
	cyng::continuation gateway_proxy::process(std::string trx,
		std::size_t idx,
		std::string server_id,
		cyng::buffer_t code,
		cyng::param_map_t params)
	{
		BOOST_ASSERT(!queue_.empty());

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> response #"
			<< queue_.front().get_sequence()
			<< " from "
			<< server_id
			<< "/"
			<< sml::from_server_id(queue_.front().get_srv())
			<< " ["
			<< queue_.front().get_msg_type()
			<< "]");

		if (server_id.empty()) {
			server_id = sml::from_server_id(queue_.front().get_srv());
		}
		else {
			//	in some cases this is the meter ident
			//BOOST_ASSERT(sml::from_server_id(queue_.front().get_srv()) == server_id);
		}

		bus_->vm_.async_run(bus_res_com_sml(queue_.front().get_tag_ident()
			, queue_.front().get_tag_source()
			, queue_.front().get_sequence()
			, queue_.front().get_key_gw()
			, queue_.front().get_tag_origin()
			, queue_.front().get_msg_type()
			, server_id
			, sml::get_name(code)
			, params));

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation gateway_proxy::process(std::string srv
		, cyng::buffer_t const& code)
	{
		sml::obis attention(code);

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> response #"
			<< queue_.front().get_sequence()
			<< " from "
			<< srv
			<< " attention code "
			<< sml::get_attention_name(attention));

		bus_->vm_.async_run(bus_res_attention_code(queue_.front().get_tag_ident()
			, queue_.front().get_tag_source()
			, queue_.front().get_sequence()
			, queue_.front().get_tag_origin()
			, srv
			, code
			, sml::get_attention_name(attention)));

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
		queue_.emplace(tag, source, seq, origin, channel,code, gw, params, srv, name, pwd, queue_.size());
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> queue size "
			<< queue_.size());

		if (queue_.size() == 1) {

			//
			//	waiting for an opportunity to open a connection.If session is ready
			//	is signals OK on slot 0
			vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));
		}
		return cyng::continuation::TASK_CONTINUE;
	}


	void gateway_proxy::execute_cmd()
	{
		//	[{("section":[op-log-status-word,root-visible-devices,root-active-devices,firmware,memory,root-wMBus-status,IF_wMBUS,root-ipt-state,root-ipt-param])}]
		//	[{("name":smf-form-gw-ipt-srv),("value":0500153B022980)},{("name":smf-gw-ipt-host-1),("value":waiting...)},{("name":smf-gw-ipt-local-1),("value":4)},{("name":smf-gw-ipt-remote-1),("value":3)},{("name":smf-gw-ipt-name-1),("value":waiting...)},{("name":smf-gw-ipt-pwd-1),("value":asdasd)},{("name":smf-gw-ipt-host-2),("value":waiting...)},{("name":smf-gw-ipt-local-2),("value":3)},{("name":smf-gw-ipt-remote-2),("value":3)},{("name":smf-gw-ipt-name-2),("value":holgär)},{("name":smf-gw-ipt-pwd-2),("value":asdasd)}]

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> execute channel "
			<< queue_.front().get_msg_type());
		
		switch (queue_.front().get_msg_code()) {

		case node::sml::BODY_GET_PROFILE_PACK_REQUEST:
		//	fall through
		case node::sml::BODY_GET_PROFILE_LIST_REQUEST:
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> not implemented yet "
				<< queue_.front().get_msg_type());
			queue_.pop();
			break;

		case node::sml::BODY_GET_PROC_PARAMETER_REQUEST:
			execute_cmd_get_proc_param();
			break;

		case node::sml::BODY_SET_PROC_PARAMETER_REQUEST:
			execute_cmd_set_proc_param();
			break;

		case node::sml::BODY_GET_LIST_REQUEST:
			execute_cmd_get_list_request();
			break;

		default:
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown channel "
				<< queue_.front().get_msg_type());

			//
			//	remove queue entry
			//
			queue_.pop();
			break;
		}
	}

	void gateway_proxy::execute_cmd_get_proc_param()
	{
		auto const server_id = queue_.front().get_srv();
		auto const user = queue_.front().get_user();
		auto const pwd = queue_.front().get_pwd();
		auto const code = queue_.front().get_root();	//	OBIS root
		auto const params = cyng::make_reader(queue_.front().get_params());

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac(), server_id, user, pwd);

		//
		//	generate get process parameter requests
		//
		if (sml::OBIS_CODE_ROOT_SENSOR_PARAMS == code || 
			sml::OBIS_CODE_ROOT_DATA_COLLECTOR == code ||
			sml::OBIS_PUSH_OPERATIONS == code) {

			//
			//	data mirror
			//	push operations
			//
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {
				sml_gen.get_proc_parameter(r.first, code, user, pwd);
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
		else {
			sml_gen.get_proc_parameter(server_id, code, user, pwd);
		}

		//
		//	generate close request
		//
		sml_gen.public_close();
		cyng::buffer_t msg = sml_gen.boxing();

		//
		//	update data throughput (outgoing)
		//
		bus_->vm_.async_run(client_inc_throughput(vm_.tag()
			, queue_.front().get_tag_source()
			, msg.size()));

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#else
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> \n"
			<< cyng::io::to_hex(msg, ' '))
#endif

		//
		//	send to gateway
		//
		vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
			, cyng::generate_invoke("stream.flush") });

	}

	void gateway_proxy::execute_cmd_set_proc_param()
	{
		auto const server_id = queue_.front().get_srv();
		auto const user = queue_.front().get_user();
		auto const pwd = queue_.front().get_pwd();
		auto const code = queue_.front().get_root();	//	OBIS root
		auto const params = cyng::make_reader(queue_.front().get_params());

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac(), server_id, user, pwd);

		//
		//	generate set process parameter requests
		//

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> set "
			<< sml::from_server_id(server_id)
			<< " root "
			<< code.to_str());

		if (sml::OBIS_CODE_ROOT_IPT_PARAM == code) {

			//
			//	modify IP-T parameters
			//

			cyng::vector_t vec;
			vec = cyng::value_cast(params.get("ipt"), vec);
			execute_cmd_set_proc_param_ipt(sml_gen, server_id, user, pwd, vec);
		}
		else if (sml::OBIS_CODE_IF_wMBUS == code) {

			//
			//	modify wireless IEC parameters
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(params.get("wmbus"), tpl);
			execute_cmd_set_proc_param_wmbus(sml_gen, server_id, user, pwd, tpl);
		}
		else if (sml::OBIS_CODE_IF_1107 == code) {

			//
			//	modify wired IEC parameters
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(params.get("iec"), tpl);
			execute_cmd_set_proc_param_iec(sml_gen, server_id, user, pwd, tpl);
		}
		else if (sml::OBIS_CODE_REBOOT == code) {

			//
			//	reboot the gateway
			//
			sml_gen.set_proc_parameter_restart(server_id, user, pwd);
		}
		else if (sml::OBIS_CODE_ACTIVATE_DEVICE == code) {

			//
			//	activate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				execute_cmd_set_proc_param_activate(sml_gen, server_id, user, pwd, nr, r.first);
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
		else if (sml::OBIS_CODE_DEACTIVATE_DEVICE == code) {

			//
			//	deactivate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				execute_cmd_set_proc_param_activate(sml_gen, server_id, user, pwd, nr, r.first);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
			execute_cmd_set_proc_param_deactivate(sml_gen, server_id, user, pwd, nr, r.first);
		}
		else if (sml::OBIS_CODE_DELETE_DEVICE == code) {

			//
			//	remove a meter device from list of visible meters
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				execute_cmd_set_proc_param_activate(sml_gen, server_id, user, pwd, nr, r.first);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
			execute_cmd_set_proc_param_delete(sml_gen, server_id, user, pwd, nr, r.first);
		}
		else if (sml::OBIS_CODE_ROOT_SENSOR_PARAMS == code) {

			//
			//	change root parameters of meter
			//
			//auto const vec = queue_.front().get_params(sec);
			//execute_cmd_set_proc_param_meter(sml_gen, server_id, user, pwd, vec);
		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown OBIS code for SetProcParameterRequest: "
				<< code.to_str());
		}

		//	generate close request
		//
		sml_gen.public_close();
		cyng::buffer_t msg = sml_gen.boxing();

		//
		//	update data throughput (outgoing)
		//
		bus_->vm_.async_run(client_inc_throughput(vm_.tag()
			, queue_.front().get_tag_source()
			, msg.size()));

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#else
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> emits \n"
			<< cyng::io::to_hex(msg, ' '))
#endif

		//
		//	send to gateway
		//
		vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
			, cyng::generate_invoke("stream.flush") });

	}

	void gateway_proxy::execute_cmd_get_list_request()
	{
		auto const server_id = queue_.front().get_srv();
		auto const user = queue_.front().get_user();
		auto const pwd = queue_.front().get_pwd();
		auto const code = queue_.front().get_root();	//	OBIS root
		auto const params = cyng::make_reader(queue_.front().get_params());

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac(), server_id, user, pwd);

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> get "
			<< sml::from_server_id(server_id)
			<< " GetListRequest: "
			<< code.to_str());

		//
		//	generate get process parameter requests
		//

		if (sml::OBIS_LIST_CURRENT_DATA_RECORD == code) {

			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {
				sml_gen.get_list_last_data_record(get_mac().to_buffer()
					, r.first
					, user
					, pwd);
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

		//	generate close request
		//
		sml_gen.public_close();
		cyng::buffer_t msg = sml_gen.boxing();

		//
		//	update data throughput (outgoing)
		//
		bus_->vm_.async_run(client_inc_throughput(vm_.tag()
			, queue_.front().get_tag_source()
			, msg.size()));

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#else
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> \n"
			<< cyng::io::to_hex(msg, ' '))
#endif

		//
		//	send to gateway
		//
		vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
			, cyng::generate_invoke("stream.flush") });

	}

	void gateway_proxy::execute_cmd_set_proc_param_ipt(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
		, cyng::vector_t vec)
	{
		std::size_t idx{ 1 };	//	index starts with 1
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
					sml_gen.set_proc_parameter_ipt_host(server_id
						, user
						, pwd
						, idx
						, address);

				}
				else if (boost::algorithm::equals(param.first, "port")) {
					auto port = cyng::numeric_cast<std::uint16_t>(param.second, 26862u);
					sml_gen.set_proc_parameter_ipt_port_local(server_id
						, user
						, pwd
						, idx
						, port);
				}
				else if (boost::algorithm::equals(param.first, "user")) {
					auto str = cyng::value_cast<std::string>(param.second, "");
					sml_gen.set_proc_parameter_ipt_user(server_id
						, user
						, pwd
						, idx
						, str);
				}
				else if (boost::algorithm::equals(param.first, "pwd")) {
					auto str = cyng::value_cast<std::string>(param.second, "");
					sml_gen.set_proc_parameter_ipt_pwd(server_id
						, user
						, pwd
						, idx
						, str);
				}
			}

			++idx;
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
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
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_INSTALL_MODE }
					, user
					, pwd
					, b);
			}
			else if (boost::algorithm::equals(param.first, "protocol")) {

				//
				//	protocol type is encodes as a single character
				//
				auto const val = cyng::value_cast<std::string>(param.second, "S");

				sml_gen.set_proc_parameter_wmbus_protocol(server_id
					, queue_.front().get_user()
					, queue_.front().get_pwd()
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
					}(val));

			}
			else if (boost::algorithm::equals(param.first, "reboot")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_REBOOT }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "sMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_MODE_S }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "tMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_MODE_T }
					, user
					, pwd
					, val);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_iec(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
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
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_ACTIVE }
					, user
					, pwd
					, b);
			}
			else if (boost::algorithm::equals(param.first, "autoActivation")) {

				const auto b = cyng::value_cast(param.second, false);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION }
					, user
					, pwd
					, b);
			}
			else if (boost::algorithm::equals(param.first, "loopTime")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 60u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_LOOP_TIME }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "maxDataRate")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MAX_DATA_RATE }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "minTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 200u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MIN_TIMEOUT }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "maxTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 5000u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MAX_TIMEOUT }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "maxVar")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 9u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MAX_VARIATION }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "protocolMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 1u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_PROTOCOL_MODE }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "retries")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 3u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_RETRIES }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "rs485")) {
				//	ignore
			}
			else if (boost::algorithm::equals(param.first, "timeGrid")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 900u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_TIME_GRID }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "timeSync")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 14400u);
				sml_gen.set_proc_parameter(server_id
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_TIME_SYNC }
					, user
					, pwd
					, val);
			}
			else if (boost::algorithm::equals(param.first, "devices")) {
				
				//
				//	ToDo:
				//
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_meter(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
		, cyng::vector_t vec)
	{
		if (vec.size() == 2) {

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
			//	DOM/param_map_t reader
			//
			auto dom = cyng::make_reader(vec);

			//
			//	extract meter: meter ID is used as server ID
			//
			auto const str = cyng::value_cast<std::string>(dom[0].get("meterId"), "");
			std::pair<cyng::buffer_t, bool> const r = sml::parse_srv_id(str);

			//
			//	modify user name
			//
			auto const new_user = cyng::value_cast<std::string>(dom[1]["data"].get("user"), "");
			sml_gen.set_proc_parameter(r.first
				, { sml::OBIS_CODE_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_NAME }
				, user
				, pwd
				, new_user);

			//
			//	modify user password
			//
			auto const new_pwd = cyng::value_cast<std::string>(dom[1]["data"].get("pwd"), "");
			sml_gen.set_proc_parameter(r.first
				, { sml::OBIS_CODE_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_PWD }
				, user
				, pwd
				, new_pwd);

			//
			//	modify public key
			//
			auto const new_pub_key = cyng::value_cast<std::string>(dom[1]["data"].get("pubKey"), "");
			auto const r_pub_key = cyng::parse_hex_string(new_pub_key);
			if (r_pub_key.second) {

				sml_gen.set_proc_parameter(r.first
					, { sml::OBIS_CODE_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_PUBLIC_KEY }
					, user
					, pwd
					, r_pub_key.first);

			}

			//
			//	modify AES key
			//
			auto const new_aes_key = cyng::value_cast<std::string>(dom[1]["data"].get("aesKey"), "");
			auto const r_aes_key = cyng::parse_hex_string(new_aes_key);
			if (r_aes_key.second) {

				sml_gen.set_proc_parameter(r.first
					, { sml::OBIS_CODE_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_AES_KEY }
					, user
					, pwd
					, r_aes_key.first);
			}

		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set meter root properties "
				<< sml::from_server_id(server_id)
				<< " has no parameters");
		}

	}

	void gateway_proxy::execute_cmd_set_proc_param_activate(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
		, std::uint8_t nr
		, cyng::buffer_t const& meter)
	{
		sml_gen.set_proc_parameter(server_id
			, { sml::OBIS_CODE_ACTIVATE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFB, nr), sml::OBIS_CODE_SERVER_ID }
			, user
			, pwd
			, meter);
	}

	void gateway_proxy::execute_cmd_set_proc_param_deactivate(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd, std::uint8_t nr
		, cyng::buffer_t const& meter)
	{
		sml_gen.set_proc_parameter(server_id
			, { sml::OBIS_CODE_DEACTIVATE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFC, nr), sml::OBIS_CODE_SERVER_ID }
			, user
			, pwd
			, meter);
	}

	void gateway_proxy::execute_cmd_set_proc_param_delete(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
		, std::uint8_t nr
		, cyng::buffer_t const& meter)
	{
		sml_gen.set_proc_parameter(server_id
			, { sml::OBIS_CODE_DELETE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFD, nr), sml::OBIS_CODE_SERVER_ID }
			, user
			, pwd
			, meter);
	}

	void gateway_proxy::stop(bool shutdown)
	{
		//
		//	terminate task
		//
		//	Be carefull when using resources from the session like vm_, 
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
