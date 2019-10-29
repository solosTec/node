/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "router.h"
#include "cache.h"

#include <smf/sml/protocol/reader.h>

#include <cyng/vm/controller.h>
#include <cyng/vm/context.h>
#include <cyng/vm/generator.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	router::router(cyng::logging::log_ptr logger
		, bool const server_mode
		, cyng::controller& vm
		, cache& db)
	: logger_(logger)
		, server_mode_(server_mode)
		, cache_(db)
		, sml_gen_()
	{
		//
		//	SML transport
		//
		vm.register_function("sml.msg", 2, std::bind(&router::sml_msg, this, std::placeholders::_1));
		vm.register_function("sml.eom", 2, std::bind(&router::sml_eom, this, std::placeholders::_1));
		vm.register_function("sml.log", 1, [this](cyng::context& ctx) {
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.log - " << cyng::value_cast<std::string>(frame.at(0), ""));
		});

		//
		//	SML data
		//
		//vm.register_function("sml.public.open.request", 6, std::bind(&router::sml_public_open_request, this, std::placeholders::_1));
		//vm.register_function("sml.public.close.request", 1, std::bind(&router::sml_public_close_request, this, std::placeholders::_1));
		//vm.register_function("sml.public.close.response", 1, std::bind(&router::sml_public_close_response, this, std::placeholders::_1));

		//vm.register_function("sml.get.proc.parameter.request", 6, std::bind(&router::sml_get_proc_parameter_request, this, std::placeholders::_1));
		//vm.register_function("sml.set.proc.parameter.request", 6, std::bind(&router::sml_set_proc_parameter_request, this, std::placeholders::_1));
		//vm.register_function("sml.get.profile.list.request", 8, std::bind(&router::sml_get_profile_list_request, this, std::placeholders::_1));

	}

	void router::sml_msg(cyng::context& ctx)
	{
		//	[{31383035323232323535353231383235322D31,0,0,{0100,{null,005056C00008,3230313830353232323235353532,0500153B02297E,6F70657261746F72,6F70657261746F72,null}},9c91},0]
		//	[{31383035323232323535353231383235322D32,1,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{810060050000},null}},f8b5},1]
		//	[{31383035323232323535353231383235322D33,2,0,{0500,{0500153B02297E,6F70657261746F72,6F70657261746F72,{8181C78201FF},null}},4d37},2]
		//	[{31383035323232323535353231383235322D34,0,0,{0200,{null}},e6e8},3]
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		//
		//	add generated instruction vector
		//
		ctx.queue(sml::reader::read(msg));
	}

	void router::sml_eom(cyng::context& ctx)
	{
		//[4aa5,4]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_INFO(logger_, "sml.eom #" << idx);

		//
		//	build SML message frame
		//
		cyng::buffer_t buf = sml_gen_.boxing();

#ifdef SMF_IO_DEBUG
		cyng::io::hex_dump hd;
		std::stringstream ss;
		hd(ss, buf.begin(), buf.end());
		CYNG_LOG_TRACE(logger_, "response:\n" << ss.str());
#endif

		//
		//	transfer data
		//	"stream.serialize"
		//
		ctx.queue(server_mode_ 
			? cyng::generate_invoke("stream.serialize", buf) 
			: cyng::generate_invoke("ipt.transfer.data", buf));

		ctx.queue(cyng::generate_invoke("stream.flush"));

		//
		//	reset generator
		//
		sml_gen_.reset();
	}

}
