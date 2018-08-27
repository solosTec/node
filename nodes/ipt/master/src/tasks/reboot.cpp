/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "reboot.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <cyng/vm/generator.h>

namespace node
{
	reboot::reboot(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag_remote
		, std::uint64_t seq_cluster		//	cluster seq
		, cyng::buffer_t const& server_id	//	server id
		, std::string user
		, std::string pwd
		, boost::uuids::uuid tag_ctx)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, tag_remote_(tag_remote)
		, server_id_(server_id)
		, user_(user)
		, pwd_(pwd)
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
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
		if (!is_waiting_)
		{
			//
			//	update task state
			//
			is_waiting_ = true;

			//
			//	send reboot sequence
			//
			send_reboot_cmd();

			return cyng::continuation::TASK_CONTINUE;
		}

		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> timeout");

		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation reboot::process()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> session closed");

		//
		//	session already stopped
		//
		//response_ = ipt::ctrl_res_login_public_policy::GENERAL_ERROR;
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

	//	slot 0
	cyng::continuation reboot::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< tag_remote_
			//<< " received response ["
			//<< ipt::ctrl_res_login_public_policy::get_response_name(res)
			<< "]");

		//response_ = res;
		return cyng::continuation::TASK_STOP;
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
		//cyng::buffer_t msg = sml_gen.boxing();

#ifdef SMF_IO_LOG
		cyng::io::hex_dump hd;
		hd(std::cerr, msg.begin(), msg.end());
#endif

		//	[0000]  1b 1b 1b 1b 01 01 01 01  76 0a 33 34 35 36 34 35  ........ v.345645
		//	[0010]  30 2d 31 62 00 62 00 72  63 01 00 77 01 07 00 00  0-1b.b.r c..w....
		//	[0020]  00 00 00 00 0f 32 30 31  38 30 36 30 38 31 36 30  .....201 80608160
		//	[0030]  32 34 39 01 09 6f 70 65  72 61 74 6f 72 09 6f 70  249..ope rator.op
		//	[0040]  65 72 61 74 6f 72 01 63  8c ad 00 76 0a 33 34 35  erator.c ...v.345
		//	[0050]  36 34 35 30 2d 32 62 00  62 00 72 63 06 00 75 01  6450-2b. b.rc..u.
		//	[0060]  09 6f 70 65 72 61 74 6f  72 09 6f 70 65 72 61 74  .operato r.operat
		//	[0070]  6f 72 71 07 81 81 c7 83  82 01 73 07 81 81 c7 83  orq..... ..s.....
		//	[0080]  82 01 01 01 63 7f cc 00  76 0a 33 34 35 36 34 35  ....c... v.345645
		//	[0090]  30 2d 33 62 00 62 00 72  63 02 00 71 01 63 05 32  0-3b.b.r c..q.c.2
		//	[00a0]  00 00 00 00 1b 1b 1b 1b  1a 03 c0 ea              ........ ....

		vm_	.async_run(cyng::generate_invoke("ipt.transfer.data", sml_gen.boxing()))
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
		std::map<std::string, std::size_t> cyng::async::task<node::reboot>::slot_names_({ { "shutdown", 1 } });

	}
}