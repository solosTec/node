/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "reboot.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/reader.h>
#include <smf/cluster/generator.h>

#include <cyng/vm/generator.h>
#include <cyng/io/io_bytes.hpp>
#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

namespace node
{
	reboot::reboot(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag_remote
		, std::uint64_t seq_cluster		//	cluster seq
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

	cyng::continuation reboot::run()
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
	cyng::continuation reboot::process(boost::uuids::uuid tag)
	{
		//
		//	update task state
		//
		is_waiting_ = false;

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> send reboot command");

		BOOST_ASSERT(tag == tag_ctx_);

		//
		//	send reboot sequence
		//
		send_reboot_cmd();

		//
		//	read SML data
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 1 - EOM
	cyng::continuation reboot::process(std::uint16_t crc, std::size_t midx)
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
		//return cyng::continuation::TASK_STOP;
	}

	//	slot 2 - receive data
	cyng::continuation reboot::process(cyng::buffer_t const& data)
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
	cyng::continuation reboot::process(cyng::buffer_t trx, std::uint8_t, std::uint8_t, cyng::tuple_t msg, std::uint16_t crc)
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
	cyng::continuation reboot::process(boost::uuids::uuid pk, cyng::buffer_t trx, std::size_t)
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
	cyng::continuation reboot::process(std::string trx,
		std::size_t idx,
		std::string serverID,
		cyng::buffer_t code,
		cyng::param_map_t params)
	{	//	unused
		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[6]
	cyng::continuation reboot::process(std::string srv, cyng::buffer_t const& msg)
	{
		//	00:15:3b:02:29:7e, code: 81 81 c7 c7 fd 00
		sml::obis code(msg);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.attention.msg "
			<< srv
			<< " attention code: "
			<< sml::get_attention_name(code));

		bus_->vm_.async_run(node::bus_res_attention_code(tag_remote_
			, seq_cluster_
			, tag_ws_
			, srv
			, msg
			, sml::get_attention_name(code)));

		return cyng::continuation::TASK_STOP;
	}

	void reboot::stop()
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

	void reboot::send_reboot_cmd()
	{
		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> send reboot command (81 81 C7 83 82 01) to server "
			<< sml::from_server_id(server_id_)
			<< " - "
			<< user_
			<< ':'
			<< pwd_);


		//
		//	send 81 81 C7 83 82 01 
		//
		node::sml::req_generator sml_gen;
		sml_gen.public_open(cyng::mac48(), server_id_, user_, pwd_);
		sml_gen.set_proc_parameter_restart(server_id_, user_, pwd_);
		sml_gen.public_close();
		cyng::buffer_t msg = sml_gen.boxing();

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#endif

		//  [0010]  38 2d 31 62 00 62 00 72  63 01 00 77 01 07 00 00  8-1b.b.r c..w....
		//  [0020]  00 00 00 00 0f 32 30 31  38 30 38 32 37 32 31 32  .....201 80827212
		//  [0030]  35 34 35 08 05 00 15 3b  02 51 7e 09 6f 70 65 72  545....; .Q~.oper
		//  [0040]  61 74 6f 72 09 6f 70 65  72 61 74 6f 72 01 63 bb  ator.ope rator.c.
		//  [0050]  c5 00 76 0a 34 38 31 32  37 32 38 2d 32 62 01 62  ..v.4812 728-2b.b
		//  [0060]  00 72 63 06 00 75 08 05  00 15 3b 02 51 7e 09 6f  .rc..u.. ..;.Q~.o
		//  [0070]  70 65 72 61 74 6f 72 09  6f 70 65 72 61 74 6f 72  perator. operator
		//  [0080]  71 07 81 81 c7 83 82 01  73 07 81 81 c7 83 82 01  q....... s.......
		//  [0090]  01 01 63 c0 c4 00 76 0a  34 38 31 32 37 32 38 2d  ..c...v. 4812728-
		//  [00a0]  33 62 00 62 00 72 63 02  00 71 01 63 e6 3b 00 00  3b.b.rc. .q.c.;..
		//  [00b0]  1b 1b 1b 1b 1a 01 a8 c1                           ........

		//
		//	update data throughput (outgoing)
		//
		bus_->vm_.async_run(client_inc_throughput(tag_remote_
			, tag_ctx_
			, msg.size()));

		vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
			, cyng::generate_invoke("stream.flush") });

		//
		//	send attention codes as response
		//
		//[2018-08-27 21:25:45.14689820] INFO  17580 -- session.e951c3e7-b792-4e62-b965-5a885c4b67ba - [ipt connection received,70,bytes]
		//[2018-08-27 21:25:45.14873530] TRACE  2908 -- e951c3e7-b792-4e62-b965-5a885c4b67ba: [op:ESBA,op:IDENT,1B1B1B1B01010101760A343831323732382D3162006200726301017601070000000000000F3230313830383237323132353435080500153B02517E0162016361DC00,ipt.req.transmit.data,op:INVOKE,op:REBA]
		//[2018-08-27 21:25:45.14974420] TRACE 11248 -- write debug log ipt-rx-e951c3e7-b792-4e62-b965-5a885c4b67ba-0001.log
		//[2018-08-27 21:25:45.19014000] INFO  11248 -- session.e951c3e7-b792-4e62-b965-5a885c4b67ba - [ipt connection received,118,bytes]
		//[2018-08-27 21:25:45.19024840] INFO   2908 -- 6 ipt instructions received
		//[2018-08-27 21:25:45.19026500] TRACE  2908 -- e951c3e7-b792-4e62-b965-5a885c4b67ba: [op:ESBA,op:IDENT,760A343831323732382D32620162007263FF0174080500153B02517E078181C7C7FE038206756E61626C6520746F2066696E6420726563697069656E7420666F7220726571756573740163FD6D00760A343831323732382D3362006200726302017101633A61000000001B1B1B1B1A033C16,ipt.req.transmit.data,op:INVOKE,op:REBA]
	}

}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::reboot>::slot_names_({ { "shutdown", 1 } });

	}
}