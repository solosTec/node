/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "logic.h"

#include <cyng/io/serializer.h>
#include <cyng/vm/controller.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/dom/reader.h>
#include <cyng/json.h>
#include <cyng/sys/cpu.h>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	namespace https
	{

		logic::logic(server& srv, cyng::controller& vm, cyng::logging::log_ptr logger)
			: srv_(srv)
			, logger_(logger)
		{
			//
			//	initialize logger domain
			//
			cyng::register_logger(logger, vm);

			vm.register_function("http.session.launch", 3, [&](cyng::context& ctx) {
				//	[849a5b98-429c-431e-911d-18a467a818ca,false,127.0.0.1:61383]
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_INFO(logger, "http.session.launch " << cyng::io::to_str(frame));
			});

			vm.register_function("ws.read", 2, std::bind(&logic::ws_read, this, std::placeholders::_1));

			vm.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));

		}

		void logic::ws_read(cyng::context& ctx)
		{
			//	[c290a604-ef91-4444-b8d4-de1c61819b96,{("cmd":subscribe),("channel":sys.cpu.load),("timer":true)}]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "ws.read - " << cyng::io::to_str(frame));

			//
			//	get session tag of websocket
			//
			auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
			BOOST_ASSERT_MSG(!tag.is_nil(), "no websocket");

			//
			//	reader for JSON data
			//
			auto reader = cyng::make_reader(frame.at(1));
			const std::string cmd = cyng::value_cast<std::string>(reader.get("cmd"), "");
			const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "generic");

			if (boost::algorithm::equals(cmd, "subscribe"))
			{
				//const std::string channel = cyng::value_cast<std::string>(reader.get("channel"), "");
				CYNG_LOG_TRACE(logger_, "ws.read - subscribe channel [" << channel << "]");

				//
				//	Subscribe a table and start initial upload
				//
				srv_.get_cm().add_channel(tag, channel);

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("update")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("value", 42));

				auto msg = cyng::json::to_string(tpl);


				//
				//	send response
				//
				srv_.get_cm().push_event(channel, msg);

			}
			else if (boost::algorithm::equals(cmd, "update"))
			{
				if (boost::algorithm::equals("sys.cpu.load", channel)) {

					//
					//	query cpu load
					//
					const auto load = cyng::sys::get_total_cpu_load();

					//
					//	generate response
					//
					auto tpl = cyng::tuple_factory(
						cyng::param_factory("cmd", std::string("update")),
						cyng::param_factory("channel", channel),
						cyng::param_factory("value", load));

					//
					//	send response
					//
					auto const msg = cyng::json::to_string(tpl);
					srv_.get_cm().push_event(channel, msg);
				}
				else {

					//
					//	generate response
					//
					auto tpl = cyng::tuple_factory(
						cyng::param_factory("cmd", std::string("update")),
						cyng::param_factory("channel", channel),
						cyng::param_factory("value", "unknown channel"));

					//
					//	send response
					//
					auto const msg = cyng::json::to_string(tpl);
					srv_.get_cm().push_event(channel, msg);
				}
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown command " << cmd);
			}

		}

	}
}
