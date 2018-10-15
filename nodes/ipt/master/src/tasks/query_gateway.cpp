/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "query_gateway.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/reader.h>
#include <smf/cluster/generator.h>
#include <smf/sml/status.h>

#include <cyng/vm/generator.h>
#include <cyng/io/io_bytes.hpp>
#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

namespace node
{
	query_gateway::query_gateway(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag_remote
		, std::uint64_t seq_cluster		//	cluster seq
		, std::vector<std::string> params
		, boost::uuids::uuid tag_ws
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
		, params_(params)
		, tag_ws_(tag_ws)
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
		, is_waiting_(true)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running: "
			<< sml::from_server_id(server_id));

	}

	cyng::continuation query_gateway::run()
	{	
		if (is_waiting_)
		{
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
	cyng::continuation query_gateway::process(boost::uuids::uuid tag)
	{
		//
		//	update task state
		//
		is_waiting_ = false;

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


	void query_gateway::stop()
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

	//	slot 1 - EOM
	cyng::continuation query_gateway::process(std::uint16_t crc, std::size_t midx)
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
	cyng::continuation query_gateway::process(cyng::buffer_t const& data)
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
	cyng::continuation query_gateway::process(cyng::buffer_t trx, std::uint8_t, std::uint8_t, cyng::tuple_t msg, std::uint16_t crc)
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
	cyng::continuation query_gateway::process(boost::uuids::uuid pk, cyng::buffer_t trx, std::size_t)
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
	cyng::continuation query_gateway::process(cyng::buffer_t const& srv
		, std::uint32_t status)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.get.proc.status.word - "
			<< status);

		sml::status word;
		word.reset(status);

		//
		//	create status word to convert into attribute map
		//
		bus_->vm_.async_run(bus_res_query_status_word(tag_remote_
			, seq_cluster_
			, tag_ws_
			, sml::from_server_id(srv)
			, sml::to_attr_map(word)));

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[6]
	cyng::continuation query_gateway::process(bool active
		, cyng::buffer_t const& srv
		, std::uint16_t nr
		, cyng::buffer_t const& meter
		, cyng::buffer_t const& dclass
		, std::chrono::system_clock::time_point st)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> #"
			<< nr
			<< " - "
			<< sml::from_server_id(meter));

		//std::uint32_t sml::get_srv_type(meter);

		BOOST_ASSERT(server_id_ == srv);
		if (active) {
			bus_->vm_.async_run(bus_res_query_srv_active(tag_remote_
				, seq_cluster_
				, tag_ws_
				, nr
				, sml::from_server_id(srv)
				, sml::from_server_id(meter)
				, std::string(dclass.begin(), dclass.end())
				, st
				, sml::get_srv_type(meter)));

		}
		else {
			bus_->vm_.async_run(bus_res_query_srv_visible(tag_remote_
				, seq_cluster_
				, tag_ws_
				, nr
				, sml::from_server_id(srv)
				, sml::from_server_id(meter)
				, std::string(dclass.begin(), dclass.end())
				, st
				, sml::get_srv_type(meter)));
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[5]
	cyng::continuation query_gateway::process(cyng::buffer_t const& srv
		, std::uint32_t nr
		, cyng::buffer_t const& section
		, cyng::buffer_t const& version
		, bool active)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> #"
			<< +nr
			<< " - "
			<< std::string(version.begin(), version.end()));

		BOOST_ASSERT(server_id_ == srv);
		bus_->vm_.async_run(node::bus_res_query_firmware(tag_remote_
			, seq_cluster_
			, tag_ws_
			, nr
			, sml::from_server_id(srv)
			, std::string(section.begin(), section.end())
			, std::string(version.begin(), version.end())
			, active));

		return cyng::continuation::TASK_CONTINUE;
	}

	void query_gateway::send_query_cmd()
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
		//	generate get process parameter requests
		//
		for (auto const& p : params_) {

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> query parameter "
				<< p);

			if (boost::algorithm::equals("status:word", p)) {
				sml_gen.get_proc_status_word(server_id_, user_, pwd_);
			}
			else if (boost::algorithm::equals("srv:visible", p)) {
				sml_gen.get_proc_parameter_srv_visible(server_id_, user_, pwd_);
			}
			else if (boost::algorithm::equals("srv:active", p)) {
				//	send 81 81 11 06 FF FF
				sml_gen.get_proc_parameter_srv_active(server_id_, user_, pwd_);
			}
			else if (boost::algorithm::equals("firmware", p)) {
				sml_gen.get_proc_parameter_firmware(server_id_, user_, pwd_);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> unknown parameter "
					<< p);
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
		vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg)), cyng::generate_invoke("stream.flush") });

	}


}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::query_gateway>::slot_names_({ 
			{ "ack", 0 },
			{ "shutdown", 1 }
		});

	}
}