/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "parser_wmbus.h"
#include "../cache.h"

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>

#include <iostream>

namespace node
{
	parser_wmbus::parser_wmbus(cyng::async::base_task* btp
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

	cyng::continuation parser_wmbus::run()
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

	void parser_wmbus::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation parser_wmbus::process(cyng::buffer_t data, std::size_t msg_counter)
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
		//  feed the parser
		//
		parser_.read(data.cbegin(), data.cend());

		//
		//	signal LED
		//	ToDo: 
		//
		//if (cyng::async::NO_TASK != task_gpio_) {
		//	base_.mux_.post(task_gpio_, 1, cyng::tuple_factory(std::uint32_t(150), std::size_t(bytes_transferred / 10)));
		//}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation parser_wmbus::process(bool b)
	{
		cache_.set_status_word(sml::STATUS_BIT_MBUS_IF_AVAILABLE, b);

		if (b) {
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> wireless M-Bus interface available");
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> wireless M-Bus interface closed");
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

