/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "proxy_comm.h"
#include "session_state.h"

#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
//#include <cyng/vector_cast.hpp>
//#include <cyng/numeric_cast.hpp>
//#include <cyng/dom/reader.h>
//#include <cyng/parser/buffer_parser.h>
//#include <cyng/factory/factory.hpp>
//
//#include <boost/algorithm/string.hpp>

namespace node 
{
	proxy_comm::proxy_comm(ipt::session_state& st)
		: state_(st)
	{}

	void proxy_comm::register_this(cyng::controller& vm)
	{
		//
		//	SML communication
		//
		vm.register_function("sml.msg", 2, std::bind(&proxy_comm::sml_msg, this, std::placeholders::_1));
		vm.register_function("sml.eom", 2, std::bind(&proxy_comm::sml_eom, this, std::placeholders::_1));
		vm.register_function("sml.public.open.response", 6, std::bind(&proxy_comm::sml_public_open_response, this, std::placeholders::_1));
		vm.register_function("sml.public.close.response", 3, std::bind(&proxy_comm::sml_public_close_response, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.srv.visible", 8, std::bind(&proxy_comm::sml_get_proc_param_srv_visible, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.srv.active", 8, std::bind(&proxy_comm::sml_get_proc_param_srv_active, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.firmware", 8, std::bind(&proxy_comm::sml_get_proc_param_firmware, this, std::placeholders::_1));
		//vm.register_function("sml.get.proc.param.simple", 6, std::bind(&proxy_comm::sml_get_proc_param_simple, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.status.word", 6, std::bind(&proxy_comm::sml_get_proc_param_status_word, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.memory", 7, std::bind(&proxy_comm::sml_get_proc_param_memory, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.wmbus.status", 9, std::bind(&proxy_comm::sml_get_proc_param_wmbus_status, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.wmbus.config", 11, std::bind(&proxy_comm::sml_get_proc_param_wmbus_config, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.ipt.state", 8, std::bind(&proxy_comm::sml_get_proc_param_ipt_status, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.ipt.param", 11, std::bind(&proxy_comm::sml_get_proc_param_ipt_param, this, std::placeholders::_1));
		vm.register_function("sml.get.proc.param.ipt.param", 11, std::bind(&proxy_comm::sml_get_proc_param_ipt_param, this, std::placeholders::_1));
		vm.register_function("sml.get.list.response", 9, std::bind(&proxy_comm::sml_get_list_response, this, std::placeholders::_1));
		vm.register_function("sml.attention.msg", 6, std::bind(&proxy_comm::sml_attention_msg, this, std::placeholders::_1));
	}

	void proxy_comm::sml_msg(cyng::context& ctx)
	{
		//
		//	1. tuple containing the SML data tree
		//	2. message index
		//

		cyng::vector_t const frame = ctx.get_frame();

		//
		//	print sml message number
		//
		std::size_t const idx = cyng::value_cast<std::size_t>(frame.at(1), 0);

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		state_.react(ipt::state::evt_sml_msg(idx, msg));
	}

	void proxy_comm::sml_eom(cyng::context& ctx)
	{
		//	[5213,3]
		//
		//	* [u16] CRC
		//	* [size_t] message counter
		//

		state_.react(ipt::state::evt_sml_eom(cyng::to_tuple(ctx.get_frame())));
	}

	void proxy_comm::sml_public_open_response(cyng::context& ctx)
	{
		//	 [6f5fade9-5364-4c7c-b8e3-cb74f16361d8,,0,000000000000,0500153B0223B3,20180828184625]
		//
		//	* [uuid] pk
		//	* [buffer] trx
		//	* [size_t] idx
		//	* clientId
		//	* serverId
		//	* reqFileId

		//const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(sr_.logger_, "sml.public.open.response "
		//	<< *this
		//	<< " - "
		//	<< cyng::io::to_str(frame));

	}

	void proxy_comm::sml_public_close_response(cyng::context& ctx)
	{
		//	 [d937611f-1d91-41c0-84e7-52f0f6ab36bc,,0]
		//
		//	* [uuid] pk
		//	* [buffer] trx
		//	* [size_t] idx

		state_.react(ipt::state::evt_sml_public_close_response(cyng::to_tuple(ctx.get_frame())));

	}

	void proxy_comm::sml_get_proc_param_srv_visible(cyng::context& ctx)
	{
		//	[63b54276-1872-484a-b282-1de45d045f58,7027958-2,0,0500153B0223B3,000c,01E61E29436587BF03,2D2D2D,2018-08-29 17:59:22.00000000]
		//
		//	* [uuid] pk
		//	* [string] trx
		//	* [size_t] idx
		//	* [buffer] server id
		//	* [buffer] OBIS code
		//	* [u16] number
		//	* [buffer] meter ID
		//	* [buffer] device class (always "---")
		//	* [timestamp] UTC/last status message

		state_.react(ipt::state::evt_sml_get_proc_param_srv_visible(ctx.get_frame()));

	}

	void proxy_comm::sml_get_proc_param_srv_active(cyng::context& ctx)
	{
		//	[63b54276-1872-484a-b282-1de45d045f58,7027958-2,0,0500153B0223B3,000c,01E61E29436587BF03,2D2D2D,2018-08-29 17:59:22.00000000]
		//
		//	* [uuid] pk
		//	* [string] trx
		//	* [size_t] idx
		//	* [buffer] server id
		//	* [u16] number
		//	* [buffer] meter ID
		//	* [buffer] device class (always "---")
		//	* [timestamp] UTC/last status message

		state_.react(ipt::state::evt_sml_get_proc_param_srv_active(ctx.get_frame()));

	}

	void proxy_comm::sml_get_proc_param_firmware(cyng::context& ctx)
	{
		//	[63b54276-1872-484a-b282-1de45d045f58,7027958-2,0,0500153B0223B3,000c,01E61E29436587BF03,2D2D2D,2018-08-29 17:59:22.00000000]
		//
		//	* [uuid] pk
		//	* [string] trx
		//	* [size_t] idx
		//	* [buffer] server id
		//	* [u8] number
		//	* [buffer] firmware name/section
		//	* [buffer] version
		//	* [bool] active/inactive

		state_.react(ipt::state::evt_sml_get_proc_param_firmware(ctx.get_frame()));
	}

	void proxy_comm::sml_get_proc_param_status_word(cyng::context& ctx)
	{
		//	[b1913f62-6cb3-4586-ad43-24b20f0e7d7e,3616596-2,0,0500153B02297E,810060050000,cc070202]
		//
		//	* [uuid] pk
		//	* [string] trx
		//	* [size_t] idx
		//	* [buffer] server id
		//	* [buffer] OBIS code (81 00 60 05 00 00)
		//	* [u32] status word

		state_.react(ipt::state::evt_sml_get_proc_param_status_word(ctx.get_frame()));

	}

	void proxy_comm::sml_get_proc_param_memory(cyng::context& ctx)
	{
		//	[eef2de9e-775a-4819-95f2-665b95da1078,9030605-2,0,0500153B01EC46,0080800010FF,14,0]
		//	
		//	* [uuid] pk
		//	* [string] trx
		//	* [size_t] idx
		//	* [buffer] server id
		//	* [buffer] OBIS code (81 00 60 05 00 00)
		//	* [u8] mirror
		//	* [u8] tmp

		state_.react(ipt::state::evt_sml_get_proc_param_memory(ctx.get_frame()));

	}

	void proxy_comm::sml_get_proc_param_wmbus_status(cyng::context& ctx)
	{
		//	 [c5870024-e64c-43dd-aac4-3f3209a291b9,7228963-2,0,0500153B021774,81060F0600FF,RC1180-MBUS3,A815919638040131,332E3038,322E3030]
		//
		//	* [uuid] pk
		//	* [string] trx
		//	* [size_t] idx
		//	* [buffer] server id
		//	* [buffer] OBIS
		//	* [string] manufacturer
		//	* [buffer] device ID
		//	* [string] firmware version
		//	* [string] hardware version
		//

		state_.react(ipt::state::evt_sml_get_proc_param_wmbus_status(ctx.get_frame()));

	}

	void proxy_comm::sml_get_proc_param_wmbus_config(cyng::context& ctx)
	{
		//	[0cbb85fa-7c66-4f5f-891e-9a409c054137,1701175-3,0,0500153B01EC46,8106190700FF,0,1e,14,00015180,0,null]

		state_.react(ipt::state::evt_sml_get_proc_param_wmbus_config(ctx.get_frame()));
	}

	void proxy_comm::sml_get_proc_param_ipt_status(cyng::context& ctx)
	{
		//	 [97197e6c-a8cb-496a-8749-6bf25867758f,1360299-2,0,00:15:3b:01:ec:46,81490D0600FF,1501a8c0,68ee,10c6]

		state_.react(ipt::state::evt_sml_get_proc_param_ipt_status(ctx.get_frame()));

	}

	void proxy_comm::sml_get_proc_param_ipt_param(cyng::context& ctx)
	{
		//	 [1b2164c5-ee0f-44d4-a4eb-6f3a78e47767,0980651-3,0,00:15:3b:02:23:b3,81490D0700FF,2,1501a8c0,68ef,0,LSMTest4,LSMTest4]

		state_.react(ipt::state::evt_sml_get_proc_param_ipt_param(ctx.get_frame()));
	}

	void proxy_comm::sml_get_list_response(cyng::context& ctx)
	{
		//	[b583e91b-14f5-4691-808a-5b0a517eb1d6,7531511-2,0,,01E61E130900163C07,%(("08 00 01 00 00 ff":0.758),("08 00 01 02 00 ff":0.758)),null,06975265]

		state_.react(ipt::state::evt_sml_get_list_response(ctx.get_frame()));
	}

	void proxy_comm::sml_attention_msg(cyng::context& ctx)
	{
		//	[7020aae0-7cbe-46a0-aaf6-3aa4b037a35e,9858052-2,0,0500153B02297E,8181C7C7FD00,null]

		state_.react(ipt::state::evt_sml_attention_msg(ctx.get_frame()));
	}


}


