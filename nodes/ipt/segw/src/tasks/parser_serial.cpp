/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "parser_serial.h"
#include "../cache.h"

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <iostream>

namespace node
{
	parser_serial::parser_serial(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cache& cfg)
	: base_(*btp) 
		, logger_(logger)
		, cache_(cfg)
		, parser_([&](cyng::vector_t&& prg) {

			//CYNG_LOG_DEBUG(logger_, prg.size() << " m-bus instructions received");
			//CYNG_LOG_TRACE(logger_, vm.tag() << ": " << cyng::io::to_str(prg));
			vm.async_run(std::move(prg));

		})
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation parser_serial::run()
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

	void parser_serial::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation parser_serial::process(cyng::buffer_t data, std::size_t msg_counter)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received msg #"
			<< msg_counter
			<< " with "
			<< cyng::bytes_to_str(data.size()));

		//
		//	cleanup
		//
		if(std::all_of(data.cbegin(), data.cend(), [](char c) { 
			return c == 0xE5; 
			})) {

			//
			//	show the parser only one E5
			//
			data = cyng::make_buffer({ 0xE5 });

		}

		//
		//  feed the parser
		//
		parser_.read(data.cbegin(), data.cend());

		//
		//	signal LED
		//	ToDo: 
		//

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation parser_serial::process(bool b)
	{
		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}


}

