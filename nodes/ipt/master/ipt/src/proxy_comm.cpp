/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "proxy_comm.h"
#include "session_state.h"
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/reader.h>

#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vm/generator.h>

namespace node 
{
	proxy_comm::proxy_comm(ipt::session_state& st, cyng::controller& vm)
		: state_(st)
		, vm_(vm)
	{
		register_this();
	}

	void proxy_comm::register_this()
	{
		//
		//	SML communication
		//
		vm_.register_function("sml.msg", 3, std::bind(&proxy_comm::sml_msg, this, std::placeholders::_1));
		vm_.register_function("sml.eom", 2, std::bind(&proxy_comm::sml_eom, this, std::placeholders::_1));
		vm_.register_function("sml.public.open.response", 4, std::bind(&proxy_comm::sml_public_open_response, this, std::placeholders::_1));
		vm_.register_function("sml.public.close.response", 1, std::bind(&proxy_comm::sml_public_close_response, this, std::placeholders::_1));
		vm_.register_function("sml.get.proc.parameter.response", 5, std::bind(&proxy_comm::sml_get_proc_param_response, this, std::placeholders::_1));
		vm_.register_function("sml.get.list.response", 5, std::bind(&proxy_comm::sml_get_list_response, this, std::placeholders::_1));
		vm_.register_function("sml.get.profile.list.response", 8, std::bind(&proxy_comm::sml_get_profile_list_response, this, std::placeholders::_1));
		vm_.register_function("sml.attention.msg", 4, std::bind(&proxy_comm::sml_attention_msg, this, std::placeholders::_1));

	}

	void proxy_comm::sml_msg(cyng::context& ctx)
	{
		//
		//	1. tuple containing the SML data tree
		//	2. message index
		//

		//	[{303637373732342D31,0,0,{0101,{null,E3D360ED0B1D,3230313930393035313531353230,0500153B02297E,null,null}},86e8},0]
		auto const frame = ctx.get_frame();

		//
		//	print sml message number and data frame
		//
		std::size_t const idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		//ctx.queue(cyng::generate_invoke("log.msg.debug", ctx.get_name(), "(idx: ", idx, ", frame: ", frame, ")"));

		//
		//	get message body
		//
		auto const msg = cyng::to_tuple(frame.at(0));

		if (msg.size() == 5) {
			//
			//	This effectively converts the SML message into function calls
			//	into the VM.
			//
			ctx.queue(sml::reader::read(msg));
		}
		else {
			auto const s = cyng::io::to_type(msg);
			ctx.queue(cyng::generate_invoke("log.msg.error", ctx.get_name(), " - wrong message size:\n", s));
		}
	}

	void proxy_comm::sml_eom(cyng::context& ctx)
	{
		//	[5213,3]
		//
		//	* [u16] CRC
		//	* [size_t] message counter
		//

		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
		state_.react(ipt::state::evt_sml_eom(cyng::to_tuple(frame)));
	}

	void proxy_comm::sml_public_open_response(cyng::context& ctx)
	{
		//	 [9456398-1,BEA14A6E6596,0500153B02297E,20190905135243]
		//
		//	* [buffer] trx
		//	* clientId
		//	* serverId
		//	* reqFileId

		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
	}

	void proxy_comm::sml_public_close_response(cyng::context& ctx)
	{
		//	 [4762494-3]
		//
		//	* [buffer] trx

		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

		state_.react(ipt::state::evt_sml_public_close_response(frame));

	}

	void proxy_comm::sml_get_proc_param_response(cyng::context& ctx)
	{
		//	slot [3]
		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

		//
		// * [string] trx
		// * [u8] group number
		// * [buffer] server id
		// * [path] path (OBIS)
		// * [tuple] SML tree
		//

		state_.react(ipt::state::evt_sml_get_proc_param_response(ctx.get_frame()));
	}

	void proxy_comm::sml_get_list_response(cyng::context& ctx)
	{
		//	slot [5]

		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
		state_.react(ipt::state::evt_sml_get_list_response(frame));
	}

	void proxy_comm::sml_get_profile_list_response(cyng::context& ctx)
	{
		//	[2159338-2,084e0f4a,0384,084dafce,<!18446744073709551615:user-defined>,0500153B021774,00070202,%(("010000090B00":{7,0,2019-10-10 15:23:50.00000000,null}),("81040D060000":{ff,0,0,null}),("810417070000":{ff,0,0,null}),("81041A070000":{ff,0,0,null}),("81042B070000":{fe,0,0,null}),("8181000000FF":{ff,0,818100000001,null}),("8181C789E2FF":{ff,0,00800000,null}))]]

		//	* trx
		//	* actTime
		//	* regPeriod
		//	* valTime
		//	* path (OBIS)
		//	* server id
		//	* status
		//	* params

		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
		state_.react(ipt::state::evt_sml_get_profile_list_response(frame));
	}

	void proxy_comm::sml_attention_msg(cyng::context& ctx)
	{
		//	[]
		auto const frame = ctx.get_frame();
		ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));
		state_.react(ipt::state::evt_sml_attention_msg(frame));
	}
}
