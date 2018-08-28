/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "query_srv_active.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/reader.h>

#include <cyng/vm/generator.h>
#include <cyng/io/io_bytes.hpp>
#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

namespace node
{
	query_srv_active::query_srv_active(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag_remote
		, std::uint64_t seq_cluster		//	cluster seq
		, cyng::buffer_t const& server_id	//	server id
		, std::string user
		, std::string pwd
		, std::chrono::seconds timeout
		, boost::uuids::uuid tag_ctx)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, tag_remote_(tag_remote)
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

	cyng::continuation query_srv_active::run()
	{	
		if (is_waiting_)
		{
			//
			//	waiting for an opportunity to open a connection. If session is ready
			//	is signals OK on slot 0
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));

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
	cyng::continuation query_srv_active::process(boost::uuids::uuid tag)
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
		//	next step: send command
		//
		send_query_cmd();

		//
		//	read SML data
		//
		return cyng::continuation::TASK_CONTINUE;
	}


	void query_srv_active::stop()
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

	//	slot 1 - shutdown
	cyng::continuation query_srv_active::process()
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(server_id_)
			<< " shutdown");

		return cyng::continuation::TASK_STOP;
	}

	//	slot 2 - receive data
	cyng::continuation query_srv_active::process(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received "
			<< cyng::bytes_to_str(data.size()));

		parser_.read(data.begin(), data.end());

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation query_srv_active::process(cyng::buffer_t trx, std::uint8_t, std::uint8_t, cyng::tuple_t msg, std::uint16_t crc)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.msg "
			<< std::string(trx.begin(), trx.end()));

		sml::reader reader;
		reader.set_trx(trx);
		vm_.async_run(reader.read(msg, 0));

		return cyng::continuation::TASK_CONTINUE;
	}

	void query_srv_active::send_query_cmd()
	{
		//
		//	send 81 81 11 06 FF FF
		//
		node::sml::req_generator sml_gen;
		sml_gen.public_open(cyng::mac48(), server_id_, user_, pwd_);
		sml_gen.get_proc_parameter_srv_active(server_id_, user_, pwd_);
		sml_gen.public_close();
		cyng::buffer_t msg = sml_gen.boxing();

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#endif

		vm_.async_run(cyng::generate_invoke("ipt.transfer.data", std::move(msg)))
			.async_run(cyng::generate_invoke("stream.flush"));


	}

}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::query_srv_active>::slot_names_({ 
			{ "ack", 0 },
			{ "shutdown", 1 }
		});

	}
}