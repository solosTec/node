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

		//
		//	update data throughput (incoming)
		//
		//bus_->vm_.async_run(client_inc_throughput(tag_ctx_
		//	, tag_remote_
		//	, data.size()));

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
			<< sml::from_server_id(queue_.front().get_srv()));

		//BOOST_ASSERT(sml::from_server_id(queue_.front().get_srv()) == server_id);
		if (server_id.empty()) {
			server_id = sml::from_server_id(queue_.front().get_srv());
		}

		bus_->vm_.async_run(bus_res_gateway_proxy(queue_.front().get_ident_tag()
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
		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac()
			, queue_.front().get_srv()
			, queue_.front().get_user()
			, queue_.front().get_pwd());

		//
		//	generate get process parameter requests
		//
		auto const sections = queue_.front().get_section_names();
		for (auto const& sec : sections) {

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> query parameter "
				<< sec);

			if (boost::algorithm::equals("op-log-status-word", sec)) {
				sml_gen.get_proc_status_word(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-visible-devices", sec)) {
				sml_gen.get_proc_parameter_srv_visible(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-active-devices", sec)) {
				//	send 81 81 11 06 FF FF
				sml_gen.get_proc_parameter_srv_active(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-device-id", sec)) {
				sml_gen.get_proc_parameter_firmware(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-memory-usage", sec)) {
				sml_gen.get_proc_parameter_memory(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-wMBus-status", sec)) {
				sml_gen.get_proc_parameter_wireless_mbus_status(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("IF_wMBUS", sec)) {
				sml_gen.get_proc_parameter_wireless_mbus_config(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-ipt-state", sec)) {
				sml_gen.get_proc_parameter_ipt_status(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
			}
			else if (boost::algorithm::equals("root-ipt-param", sec)) {
				sml_gen.get_proc_parameter_ipt_config(queue_.front().get_srv(), queue_.front().get_user(), queue_.front().get_pwd());
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
		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac()
			, queue_.front().get_srv()
			, queue_.front().get_user()
			, queue_.front().get_pwd());

		//
		//	generate get process parameter requests
		//
		auto const sections = queue_.front().get_section_names();
		BOOST_ASSERT_MSG(sections.size() == 1, "one section expected");
		for (auto const& sec : sections) {

			if (boost::algorithm::equals(sec, "ipt")) {
				execute_cmd_set_proc_param_ipt(sml_gen);
			}
			else if (boost::algorithm::equals(sec, "wmbus")) {
				execute_cmd_set_proc_param_wmbus(sml_gen);
			}
			else if (boost::algorithm::equals(sec, "reboot")) {
				sml_gen.set_proc_parameter_restart(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd());
			}
			//else if (boost::algorithm::equals(sec, "query")) {
			//	execute_cmd_get_list_req_last_data_set(sml_gen);
			//}
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
			<< "> \n"
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
		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;

		sml_gen.public_open(get_mac()
			, queue_.front().get_srv()
			, queue_.front().get_user()
			, queue_.front().get_pwd());

		//
		//	generate get process parameter requests
		//
		auto const sections = queue_.front().get_section_names();
		BOOST_ASSERT_MSG(sections.size() == 1, "one section expected");
		for (auto const& sec : sections) {

			if (boost::algorithm::equals(sec, "list-current-data-record")) {
				execute_cmd_get_list_req_last_data_set(sml_gen);
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


	void gateway_proxy::execute_cmd_set_proc_param_ipt(sml::req_generator& sml_gen)
	{
		//	("smf-gw-ipt-host-1":192.168.1.21),("smf-gw-ipt-host-2":192.168.1.21),("smf-gw-ipt-local-1":68ee),("smf-gw-ipt-local-2":68ef),("smf-gw-ipt-name-1":LSMTest5),("smf-gw-ipt-name-2":werwer),("smf-gw-ipt-pwd-1":LSMTest5),("smf-gw-ipt-pwd-2":LSMTest5),("smf-gw-ipt-remote-1":0),("smf-gw-ipt-remote-2":0))]
		//	{("name":smf-form-gw-ipt-srv),("value":0500153B022980)},{("name":smf-gw-ipt-host-1),("value":waiting...)},{("name":smf-gw-ipt-local-1),("value":4)},{("name":smf-gw-ipt-remote-1),("value":3)},{("name":smf-gw-ipt-name-1),("value":waiting...)},{("name":smf-gw-ipt-pwd-1),("value":asdasd)},{("name":smf-gw-ipt-host-2),("value":waiting...)},{("name":smf-gw-ipt-local-2),("value":3)},{("name":smf-gw-ipt-remote-2),("value":3)},{("name":smf-gw-ipt-name-2),("value":holgär)},{("name":smf-gw-ipt-pwd-2),("value":asdasd)},{("section":[ipt])}

		auto const params  = queue_.front().get_params();
		for (auto const& p : params) {

			if (boost::algorithm::equals(p.first, "smf-gw-ipt-host-1")) {

				//	send host name as it is - string
				//boost::asio::ip::address address;
				auto address = cyng::value_cast<std::string>(p.second, "0.0.0.0");
				sml_gen.set_proc_parameter_ipt_host(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 1
					, address);

			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-host-2")) {

				auto address = cyng::value_cast<std::string>(p.second, "0.0.0.0");
				sml_gen.set_proc_parameter_ipt_host(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 2
					, address);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-local-1")) {
				auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
				sml_gen.set_proc_parameter_ipt_port_local(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 1
					, port);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-local-2")) {
				auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
				sml_gen.set_proc_parameter_ipt_port_local(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 2
					, port);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-remote-1")) {
				auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
				sml_gen.set_proc_parameter_ipt_port_remote(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 1
					, port);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-remote-2")) {
				auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
				sml_gen.set_proc_parameter_ipt_port_remote(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 2
					, port);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-name-1")) {
				auto str = cyng::value_cast<std::string>(p.second, "");
				sml_gen.set_proc_parameter_ipt_user(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 1
					, str);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-name-2")) {
				auto str = cyng::value_cast<std::string>(p.second, "");
				sml_gen.set_proc_parameter_ipt_user(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 2
					, str);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-pwd-1")) {
				auto str = cyng::value_cast<std::string>(p.second, "");
				sml_gen.set_proc_parameter_ipt_pwd(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 1
					, str);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-ipt-pwd-2")) {
				auto str = cyng::value_cast<std::string>(p.second, "");
				sml_gen.set_proc_parameter_ipt_pwd(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, 2
					, str);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen)
	{
		auto const params = queue_.front().get_params();
		for (auto const& p : params) {
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set w-MBus parameter "
				<< p.first
				<< ": "
				<< cyng::io::to_str(p.second));

			// set w-MBus parameter smf-gw-wmbus-adapter: a815927472040131
			// set w-MBus parameter smf-gw-wmbus-firmware: 3.08
			// set w-MBus parameter smf-gw-wmbus-hardware: 2.00
			// set w-MBus parameter smf-gw-wmbus-install: on	
			// set w-MBus parameter smf-gw-wmbus-power: 2
			// set w-MBus parameter smf-gw-wmbus-protocol: 3
			// set w-MBus parameter smf-gw-wmbus-reboot: 86400
			// set w-MBus parameter smf-gw-wmbus-type: RC1180-MBUS3
			if (boost::algorithm::equals(p.first, "smf-gw-wmbus-install")) {

				const auto b = cyng::value_cast(p.second, false);
				sml_gen.set_proc_parameter_wmbus_install(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, b);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-wmbus-power")) {
				const auto val = cyng::value_cast<std::uint8_t>(p.second, 0u);
				sml_gen.set_proc_parameter_wmbus_power(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-wmbus-protocol")) {
				const auto val = cyng::value_cast<std::uint8_t>(p.second, 0u);
				sml_gen.set_proc_parameter_wmbus_protocol(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
			else if (boost::algorithm::equals(p.first, "smf-gw-wmbus-reboot")) {
				const auto val = cyng::value_cast<std::uint64_t>(p.second, 0u);
				sml_gen.set_proc_parameter_wmbus_reboot(queue_.front().get_srv()
					, queue_.front().get_user()
					, queue_.front().get_pwd()
					, val);
			}
		}
	}

	void gateway_proxy::execute_cmd_get_list_req_last_data_set(sml::req_generator& sml_gen)
	{
		auto const params = queue_.front().get_params();
		//CYNG_LOG_DEBUG(logger_, "task #"
		//	<< base_.get_id()
		//	<< " <"
		//	<< base_.get_class_name()
		//	<< "> get last data set - "
		//	<< params.size()
		//	<< " parameter available");

		//	[2019-01-16 12:58:57.94951330] DEBUG  2312 -- task #12 <node::gateway_proxy> get last data set meter-id: 01-a815-74314504-01-02
		//	[2019-01-16 12:58:57.95178820] DEBUG  2312 -- task #12 <node::gateway_proxy> get last data set meter-tag: e7f349a1-0900-4ee4-a7d5-1c468a5552c3
		for (auto const& p : params) {
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> get last data set "
				<< p.first
				<< ": "
				<< cyng::io::to_str(p.second));
		}

		auto const pos = params.find("meter-id");
		if (pos != params.end()) {

			//std::pair<cyng::buffer_t, bool> parse_srv_id(std::string const&);
			auto const r = sml::parse_srv_id(cyng::io::to_str(pos->second));
			if (r.second) {

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> get last data set from ["
					<< cyng::io::to_str(pos->second)
					<< "]");

				sml_gen.get_list_last_data_record(get_mac().to_buffer()	//queue_.front().get_srv()
					, r.first	//	meter
					, queue_.front().get_user()
					, queue_.front().get_pwd());
			}
			else {

				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> get last data set from has an invalid meter ID ["
					<< cyng::io::to_str(pos->second)
					<< "]");

			}
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> get last data set from "
				<< sml::from_server_id(queue_.front().get_srv())
				<< " has no 'meter-id' parameter");

		}
	}


	void gateway_proxy::stop()
	{
		//
		//	terminate task
		//
		auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< vm_.tag()
			<< " stopped after "
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
