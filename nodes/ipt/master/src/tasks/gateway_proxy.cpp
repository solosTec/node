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
			<< queue_.front().get_channel()
			<< "]");

		if (server_id.empty()) {
			server_id = sml::from_server_id(queue_.front().get_srv());
		}
		else {
			BOOST_ASSERT(sml::from_server_id(queue_.front().get_srv()) == server_id);
		}

		bus_->vm_.async_run(bus_res_com_sml(queue_.front().get_ident_tag()
			, queue_.front().get_source_tag()
			, queue_.front().get_sequence()
			, queue_.front().get_key()
			, queue_.front().get_ws_tag()
			, queue_.front().get_channel()
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

		bus_->vm_.async_run(bus_res_attention_code(queue_.front().get_ident_tag()
			, queue_.front().get_source_tag()
			, queue_.front().get_sequence()
			, queue_.front().get_ws_tag()
			, srv
			, code
			, sml::get_attention_name(attention)));

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation gateway_proxy::process(
		boost::uuids::uuid tag,		//	[0] ident tag
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		cyng::vector_t key,			//	[3] TGateway key
		boost::uuids::uuid ws_tag,	//	[4] ws tag
		std::string channel,		//	[5] channel
		cyng::vector_t sections,	//	[6] sections
		cyng::vector_t params,		//	[7] parameters
		cyng::buffer_t srv,			//	[8] server id
		std::string name,			//	[9] name
		std::string	pwd				//	[10] pwd
	)
	{
		queue_.emplace(tag, source, seq, key, ws_tag, channel, sections, params, srv, name, pwd, queue_.size());
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
			<< queue_.front().get_channel());
		
		if (boost::algorithm::equals(queue_.front().get_channel(), "get.proc.param")) {

			execute_cmd_get_proc_param();
		}
		else if (boost::algorithm::equals(queue_.front().get_channel(), "set.proc.param")) {

			execute_cmd_set_proc_param();
		}
		else if (boost::algorithm::equals(queue_.front().get_channel(), "get.list.request")) {

			execute_cmd_get_list_request();
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown channel "
				<< queue_.front().get_channel());

			//
			//	remove queue entry
			//
			queue_.pop();
		}
	}

	void gateway_proxy::execute_cmd_get_proc_param()
	{
		auto const server_id = queue_.front().get_srv();
		auto const user = queue_.front().get_user();
		auto const pwd = queue_.front().get_pwd();

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac(), server_id, user, pwd);

		//
		//	generate get process parameter requests
		//
		auto const sections = queue_.front().get_section_names();
		for (auto const& sec : sections) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> query "
				<< sml::from_server_id(server_id)
				<< " section ["
				<< sec
				<< "]");

			if (boost::algorithm::equals("op-log-status-word", sec)) {

				//
				//	81 00 60 05 00 00
				//
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CLASS_OP_LOG_STATUS_WORD, user, pwd);
			}
			else if (boost::algorithm::equals("root-visible-devices", sec)) {

				//
				//	81 81 10 06 FF FF
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_VISIBLE_DEVICES, user, pwd);
			}
			else if (boost::algorithm::equals("root-active-devices", sec)) {

				//
				//	send 81 81 11 06 FF FF
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_ACTIVE_DEVICES, user, pwd);
			}
			else if (boost::algorithm::equals("root-device-id", sec)) {

				//
				//	81 81 C7 82 FF
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_DEVICE_IDENT, user, pwd);
			}
			else if (boost::algorithm::equals("root-memory-usage", sec)) {

				//
				//	81 81 C7 82 FF
				//	
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_MEMORY_USAGE, user, pwd);
			}
			else if (boost::algorithm::equals("root-wMBus-status", sec)) {

				//
				//	81 06 0F 06 00 FF
				//
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_W_MBUS_STATUS, user, pwd);
			}
			else if (boost::algorithm::equals("IF-wireless-mbus", sec)) {

				//
				//	81 06 19 07 00 FF
				//	
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_IF_wMBUS, user, pwd);
			}
			else if (boost::algorithm::equals("root-ipt-state", sec)) {

				//
				//	81 49 0D 06 00 FF
				//
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_IPT_STATE, user, pwd);
			}
			else if (boost::algorithm::equals("root-ipt-param", sec)) {

				//
				//	81 49 0D 07 00 FF 
				//	
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_ROOT_IPT_PARAM, user, pwd);
			}
			else if (boost::algorithm::equals("IF-IEC-62505-21", sec)) {

				//
				//	81 81 C7 93 00 FF
				//
				sml_gen.get_proc_parameter(server_id, sml::OBIS_CODE_IF_1107, user, pwd);
			}
			else if (boost::algorithm::equals("root-sensor-params", sec)) {

				//
				//	81 81 C7 86 00 FF
				//

				//
				//	extract meter
				//
				auto const vec = queue_.front().get_params(sec);
				if (!vec.empty()) {
					cyng::buffer_t meter;
					meter = cyng::value_cast(vec.at(0), meter);

					sml_gen.get_proc_parameter(meter, sml::OBIS_CODE_ROOT_SENSOR_PARAMS, user, pwd);
				}
			}
			else if (boost::algorithm::equals("root-data-prop", sec)) {

				//
				//	81 81 C7 86 20 FF
				//	data mirror
				//

				auto const vec = queue_.front().get_params(sec);
				if (!vec.empty()) {
					cyng::buffer_t meter;
					meter = cyng::value_cast(vec.at(0), meter);

					sml_gen.get_proc_parameter(meter, sml::OBIS_CODE_ROOT_DATA_COLLECTOR, user, pwd);
				}
			}
			else if (boost::algorithm::equals("root-push-ops", sec)) {

				//
				//	81 81 C7 8A 01 FF
				//	push ops
				//

				auto const vec = queue_.front().get_params(sec);
				if (!vec.empty()) {
					cyng::buffer_t meter;
					meter = cyng::value_cast(vec.at(0), meter);

					sml_gen.get_proc_parameter(meter, sml::OBIS_PUSH_OPERATIONS, user, pwd);
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> unknown parameter/section for get-proc-param: "
					<< sec);
			}
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
			, queue_.front().get_source_tag()
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

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac(), server_id, user, pwd);

		//
		//	generate get process parameter requests
		//
		auto const sections = queue_.front().get_section_names();
		BOOST_ASSERT_MSG(sections.size() == 1, "one section expected");
		for (auto const& sec : sections) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set "
				<< sml::from_server_id(server_id)
				<< " parameter "
				<< sec);

			if (boost::algorithm::equals(sec, "root-ipt-param")) {

				//
				//	modify IP-T parameters
				//
				auto const vec = queue_.front().get_params(sec);
				execute_cmd_set_proc_param_ipt(sml_gen, server_id, user, pwd, vec);
			}
			else if (boost::algorithm::equals(sec, "IF-wireless-mbus")) {

				//
				//	modify wireless IEC parameters
				//
				execute_cmd_set_proc_param_wmbus(sml_gen, sec);
			}
			else if (boost::algorithm::equals(sec, "IF-IEC-62505-21")) {

				//
				//	modify wired IEC parameters
				//
				execute_cmd_set_proc_param_iec(sml_gen, sec);
			}
			else if (boost::algorithm::equals(sec, "reboot")) {

				//
				//	reboot the gateway
				//
				sml_gen.set_proc_parameter_restart(server_id, user, pwd);
			}
			else if (boost::algorithm::equals(sec, "activate")) {

				//
				//	activate meter device
				//
				execute_cmd_set_proc_param_activate(sml_gen, sec);
			}
			else if (boost::algorithm::equals(sec, "deactivate")) {

				//
				//	deactivate meter device
				//
				execute_cmd_set_proc_param_deactivate(sml_gen, sec);
			}
			else if (boost::algorithm::equals(sec, "delete")) {

				//
				//	remove a meter device from list of visible meters
				//
				execute_cmd_set_proc_param_delete(sml_gen, sec);
			}
			else if (boost::algorithm::equals(sec, "root-sensor-params")) {

				//
				//	change root parameters of meter
				//
				auto const vec = queue_.front().get_params(sec);
				execute_cmd_set_proc_param_meter(sml_gen, server_id, user, pwd, vec);
			}
			else {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> unknown section for set-proc-param: "
					<< sec);
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
			, queue_.front().get_source_tag()
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

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac(), server_id, user, pwd);

		//
		//	generate get process parameter requests
		//
		auto const sections = queue_.front().get_section_names();
		BOOST_ASSERT_MSG(sections.size() == 1, "one section expected");
		for (auto const& sec : sections) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> get "
				<< sml::from_server_id(server_id)
				<< " list "
				<< sec);

			if (boost::algorithm::equals(sec, "current-data-record")) {

				//
				//	get last/newest data set
				//
				auto const vec = queue_.front().get_params(sec);
				execute_cmd_get_list_req_last_data_set(sml_gen, server_id, user, pwd, vec);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> unknown section for get-list-request: "
					<< sec);
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
			, queue_.front().get_source_tag()
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
		//auto const vec  = queue_.front().get_params(section);
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

	void gateway_proxy::execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen, std::string const& section)
	{
		auto const vec = queue_.front().get_params(section);
		for (auto const& p : vec) {

			cyng::param_t param;
			param = cyng::value_cast(p, param);

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
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_INSTALL_MODE }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, b);
			}
			//else if (boost::algorithm::equals(p.first, "smf-gw-wmbus-power")) {
			//	const auto val = cyng::value_cast<std::uint8_t>(p.second, 0u);
			//	sml_gen.set_proc_parameter_wmbus_power(queue_.front().get_srv()
			//		, queue_.front().get_user()
			//		, queue_.front().get_pwd()
			//		, val);
			//}
			else if (boost::algorithm::equals(param.first, "protocol")) {

				//
				//	protocol type is encodes as a single character
				//
				auto const val = cyng::value_cast<std::string>(param.second, "S");

				sml_gen.set_proc_parameter_wmbus_protocol(queue_.front().get_srv()
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
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_REBOOT }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "sMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_MODE_S }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "tMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_wMBUS, sml::OBIS_W_MBUS_MODE_T }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_iec(sml::req_generator& sml_gen, std::string const& section)
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
 
		auto const vec = queue_.front().get_params(section);
		for (auto const& p : vec) {
			cyng::param_t param;
			param = cyng::value_cast(p, param);

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
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_ACTIVE }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, b);
			}
			else if (boost::algorithm::equals(param.first, "autoActivation")) {

				const auto b = cyng::value_cast(param.second, false);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, b);
			}
			else if (boost::algorithm::equals(param.first, "loopTime")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 60u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_LOOP_TIME }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "maxDataRate")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MAX_DATA_RATE }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "minTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 200u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MIN_TIMEOUT }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "maxTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 5000u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MAX_TIMEOUT }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "maxVar")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 9u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_MAX_VARIATION }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "protocolMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 1u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_PROTOCOL_MODE }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "retries")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 3u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_RETRIES }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "rs485")) {
				//	ignore
			}
			else if (boost::algorithm::equals(param.first, "timeGrid")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 900u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_TIME_GRID }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "timeSync")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 14400u);
				sml_gen.set_proc_parameter(queue_.front().get_srv()
					, sml::obis_path{ sml::OBIS_CODE_IF_1107, sml::OBIS_CODE_IF_1107_TIME_SYNC }
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(param.first, "devices")) {
				
				//
				//	ToDo:
				//
			}
		}
	}

	void gateway_proxy::execute_cmd_get_list_req_last_data_set(sml::req_generator& sml_gen
		, cyng::buffer_t const& server_id
		, std::string const& user
		, std::string const& pwd
		, cyng::vector_t vec)
	{
		if (!vec.empty()) {

			//
			//	extract meter
			//
			cyng::buffer_t meter;
			meter = cyng::value_cast(vec.at(0), meter);

			sml_gen.get_list_last_data_record(get_mac().to_buffer()
				, meter
				, user
				, pwd);
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> get last data set from "
				<< sml::from_server_id(server_id)
				<< " has no parameters");
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

	void gateway_proxy::execute_cmd_set_proc_param_activate(sml::req_generator& sml_gen, std::string const& section)
	{
		//[{ meter: item.meter }]
		auto const vec = queue_.front().get_params(section);
		if (!vec.empty()) {

			//
			//	extract meter
			//
			cyng::buffer_t meter;
			meter = cyng::value_cast(vec.at(1), meter);
			sml_gen.set_proc_parameter(queue_.front().get_srv()
				, { sml::OBIS_CODE_ACTIVATE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFB, 0x01), sml::OBIS_CODE_SERVER_ID }
				, queue_.front().get_user()
				, queue_.front().get_pwd()
				, meter);
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> [activate] has no parameters - "
				<< sml::from_server_id(queue_.front().get_srv()));
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_deactivate(sml::req_generator& sml_gen, std::string const& section)
	{
		auto const vec = queue_.front().get_params(section);
		if (!vec.empty()) {

			//
			//	extract meter
			//
			cyng::buffer_t meter;
			meter = cyng::value_cast(vec.at(1), meter);
			sml_gen.set_proc_parameter(queue_.front().get_srv()
				, { sml::OBIS_CODE_DEACTIVATE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFC, 0x01), sml::OBIS_CODE_SERVER_ID }
				, queue_.front().get_user()
				, queue_.front().get_pwd()
				, meter);
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> [deactivate] has no parameters - "
				<< sml::from_server_id(queue_.front().get_srv()));
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_delete(sml::req_generator& sml_gen, std::string const& section)
	{
		auto const vec = queue_.front().get_params(section);
		if (!vec.empty()) {

			//
			//	extract meter
			//
			cyng::buffer_t meter;
			meter = cyng::value_cast(vec.at(1), meter);
			sml_gen.set_proc_parameter(queue_.front().get_srv()
				, { sml::OBIS_CODE_DELETE_DEVICE, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFD, 0x01), sml::OBIS_CODE_SERVER_ID }
				, queue_.front().get_user()
				, queue_.front().get_pwd()
				, meter);
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> [delete] has no parameters - "
				<< sml::from_server_id(queue_.front().get_srv()));
		}
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
