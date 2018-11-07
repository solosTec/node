/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "modify_gateway.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/reader.h>
#include <smf/cluster/generator.h>
#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>

#include <cyng/vm/generator.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>

#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

namespace node
{
	modify_gateway::modify_gateway(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag_remote
		, std::uint64_t seq_cluster		//	cluster seq
		, std::string const& section
		, cyng::param_map_t const& params
		, cyng::buffer_t const& server_id	//	server id
		, std::string user
		, std::string pwd
		, std::chrono::seconds timeout
		, boost::uuids::uuid tag_ctx)
	: base_(*btp)
		, logger_(logger)
		, bus_(bus)
		, vm_(vm)
		, tag_remote_(tag_remote)
		, seq_cluster_(seq_cluster)
		, section_(section)
		, params_(params)
		, server_id_(server_id)
		, user_(user)
		, pwd_(pwd)
		, timeout_(timeout)
		, tag_ctx_(tag_ctx)
		, start_(std::chrono::system_clock::now())
		, parser_([this](cyng::vector_t&& prg) {

			CYNG_LOG_DEBUG(logger_, "sml processor - "
				<< prg.size()
				<< " instructions");

			CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false, false)	//	not verbose, no log instructions
		, wait_counter_(12)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running: "
			<< sml::from_server_id(server_id));

	}

	cyng::continuation modify_gateway::run()
	{	
		if (wait_counter_ > 0u)
		{
			wait_counter_--;

			//
			//	waiting for an opportunity to open a connection. If session is ready
			//	is signals OK on slot 0
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> waiting for "
				<< wait_counter_
				<< " x "
				<< cyng::to_str(timeout_));

			//
			//	start monitor
			//
			base_.suspend(timeout_);

			return cyng::continuation::TASK_CONTINUE;
		}

		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> timeout");

		return cyng::continuation::TASK_STOP;
	}

	//	slot 0 - ack
	cyng::continuation modify_gateway::process(boost::uuids::uuid tag)
	{
		//
		//	update task state
		//
		wait_counter_ = 0u;

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> send query");

		BOOST_ASSERT(tag == tag_ctx_);

		//
		//	next step: send command(s)
		//
		send_query_cmd();

		//
		//	read SML data
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 1 - EOM
	cyng::continuation modify_gateway::process(std::uint16_t crc, std::size_t midx)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(server_id_)
			<< " EOM #"
			<< midx);

		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 2 - receive data
	cyng::continuation modify_gateway::process(cyng::buffer_t const& data)
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
		bus_->vm_.async_run(client_inc_throughput(tag_ctx_
			, tag_remote_
			, data.size()));

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[3]
	cyng::continuation modify_gateway::process(cyng::buffer_t trx, std::uint8_t, std::uint8_t, cyng::tuple_t msg, std::uint16_t crc)
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
	cyng::continuation modify_gateway::process(boost::uuids::uuid pk, cyng::buffer_t trx, std::size_t)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.public.close.response "
			<< std::string(trx.begin(), trx.end()));

		return cyng::continuation::TASK_STOP;
	}

	//	-- slot[5]
	cyng::continuation modify_gateway::process(std::string trx,
		std::size_t idx,
		std::string server_id,
		cyng::buffer_t code,
		cyng::param_map_t params)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> response #"
			<< seq_cluster_
			<< " from "
			<< server_id);

		//bus_->vm_.async_run(bus_res_modify_gateway(tag_remote_
		//	, seq_cluster_
		//	, tag_ws_
		//	, server_id
		//	, sml::get_name(code)
		//	, params));

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation modify_gateway::process(std::string srv
		, cyng::buffer_t const& code)
	{
		sml::obis attention(code);

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> response #"
			<< seq_cluster_
			<< " from "
			<< srv
			<< " attention code "
			<< sml::get_attention_name(attention));

		//bus_->vm_.async_run(bus_res_attention_code(tag_remote_
		//	, seq_cluster_
		//	, tag_ws_
		//	, srv
		//	, code
		//	, sml::get_attention_name(attention)));

		return cyng::continuation::TASK_CONTINUE;
	}

	void modify_gateway::send_query_cmd()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> send "
			<< params_.size()
			<< " parameter request(s)");
		

		//
		//	generate public open request
		//
		node::sml::req_generator sml_gen;
		sml_gen.public_open(cyng::mac48(), server_id_, user_, pwd_);

		//
		//	generate set process parameter requests
		//
		if (boost::algorithm::equals(section_, "ipt")) {
			//	("smf-gw-ipt-host-1":192.168.1.21),("smf-gw-ipt-host-2":192.168.1.21),("smf-gw-ipt-local-1":68ee),("smf-gw-ipt-local-2":68ef),("smf-gw-ipt-name-1":LSMTest5),("smf-gw-ipt-name-2":werwer),("smf-gw-ipt-pwd-1":LSMTest5),("smf-gw-ipt-pwd-2":LSMTest5),("smf-gw-ipt-remote-1":0),("smf-gw-ipt-remote-2":0))]
			for (auto const& p : params_) {

				if (boost::algorithm::equals(p.first, "smf-gw-ipt-host-1")) {

					boost::asio::ip::address address;
					address = cyng::value_cast(p.second, address);
					sml_gen.set_proc_parameter_ipt_host(server_id_
						, user_
						, pwd_
						, 1
						, address);

				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-host-2")) {
					boost::asio::ip::address address;
					address = cyng::value_cast(p.second, address);
					sml_gen.set_proc_parameter_ipt_host(server_id_
						, user_
						, pwd_
						, 2
						, address);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-local-1")) {
					auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
					sml_gen.set_proc_parameter_ipt_port_local(server_id_
						, user_
						, pwd_
						, 1
						, port);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-local-2")) {
					auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
					sml_gen.set_proc_parameter_ipt_port_local(server_id_
						, user_
						, pwd_
						, 2
						, port);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-remote-1")) {
					auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
					sml_gen.set_proc_parameter_ipt_port_remote(server_id_
						, user_
						, pwd_
						, 1
						, port);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-remote-2")) {
					auto port = cyng::value_cast<std::uint16_t>(p.second, 26862u);
					sml_gen.set_proc_parameter_ipt_port_remote(server_id_
						, user_
						, pwd_
						, 2
						, port);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-name-1")) {
					auto str = cyng::value_cast<std::string>(p.second, "");
					sml_gen.set_proc_parameter_ipt_user(server_id_
						, user_
						, pwd_
						, 1
						, str);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-name-2")) {
					auto str = cyng::value_cast<std::string>(p.second, "");
					sml_gen.set_proc_parameter_ipt_user(server_id_
						, user_
						, pwd_
						, 2
						, str);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-pwd-1")) {
					auto str = cyng::value_cast<std::string>(p.second, "");
					sml_gen.set_proc_parameter_ipt_pwd(server_id_
						, user_
						, pwd_
						, 1
						, str);
				}
				else if (boost::algorithm::equals(p.first, "smf-gw-ipt-pwd-2")) {
					auto str = cyng::value_cast<std::string>(p.second, "");
					sml_gen.set_proc_parameter_ipt_pwd(server_id_
						, user_
						, pwd_
						, 2
						, str);
				}
			}
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> section "
				<< section_);
		}
		//for (auto const& p : params_) {

		//	CYNG_LOG_TRACE(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> query parameter "
		//		<< p);

		//	if (boost::algorithm::equals("status:word", p)) {
		//		sml_gen.get_proc_status_word(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("srv:visible", p)) {
		//		sml_gen.get_proc_parameter_srv_visible(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("srv:active", p)) {
		//		//	send 81 81 11 06 FF FF
		//		sml_gen.get_proc_parameter_srv_active(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("firmware", p)) {
		//		sml_gen.get_proc_parameter_firmware(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("memory", p)) {
		//		sml_gen.get_proc_parameter_memory(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("w-mbus-status", p)) {
		//		sml_gen.get_proc_parameter_wireless_mbus_status(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("w-mbus-config", p)) {
		//		sml_gen.get_proc_parameter_wireless_mbus_config(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("ipt-status", p)) {
		//		sml_gen.get_proc_parameter_ipt_status(server_id_, user_, pwd_);
		//	}
		//	else if (boost::algorithm::equals("ipt-config", p)) {
		//		sml_gen.get_proc_parameter_ipt_config(server_id_, user_, pwd_);
		//	}
		//	else {
		//		CYNG_LOG_WARNING(logger_, "task #"
		//			<< base_.get_id()
		//			<< " <"
		//			<< base_.get_class_name()
		//			<< "> unknown parameter "
		//			<< p);
		//	}
		//}

		//
		//	generate close request
		//
		sml_gen.public_close();
		cyng::buffer_t msg = sml_gen.boxing();

		//
		//	update data throughput (outgoing)
		//
		bus_->vm_.async_run(client_inc_throughput(tag_remote_
			, tag_ctx_
			, msg.size()));

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#endif

		//
		//	send to gateway
		//
		vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
			, cyng::generate_invoke("stream.flush") });

	}

	void modify_gateway::stop()
	{
		//
		//	terminate task
		//
		auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped: "
			<< sml::from_server_id(server_id_)
			<< " after "
			<< uptime.count()
			<< " milliseconds");
	}
}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::modify_gateway>::slot_names_({ 
			{ "ack", 0 },
			{ "shutdown", 1 }
		});

	}
}