﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "parser_CP210x.h"

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>


namespace node
{
	parser_CP210x::parser_CP210x(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cyng::async::task_list_t const& receiver)
	: base_(*btp) 
		, logger_(logger)
		, receiver_(receiver)
		, parser_([&](cyng::vector_t&& prg) {
			
			//
			//
			//
			vm.async_run(std::move(prg));
		})
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		vm.register_function("hci.payload", 2, [&](cyng::context& ctx) {

			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	send data to receiver (parser)
			//
			for (auto const tsk : receiver_) {
				base_.mux_.post(tsk, 0, cyng::to_tuple(frame));
			}
			//base_.mux_.post(receiver_, 0, cyng::tuple_factory(cyng::make_buffer({ 0x1, 0x2, 0x3 }), static_cast<std::size_t>(42u)));
		});

	}

	cyng::continuation parser_CP210x::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	ToDo: write the log entry
		//

		//
		//	start/restart timer
		//

		return cyng::continuation::TASK_CONTINUE;
	}

	void parser_CP210x::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation parser_CP210x::process(cyng::buffer_t data, std::size_t msg_counter)
	{

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received msg #"
			<< msg_counter);

		//
		//  feed the parser
		//
		parser_.read(data.cbegin(), data.cend());

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}


}

