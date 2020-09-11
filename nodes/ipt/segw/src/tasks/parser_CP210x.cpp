/*
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
		, cyng::controller& vm)
	: base_(*btp) 
		, logger_(logger)
		, receiver_()
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
			CYNG_LOG_TRACE(logger_, ctx.get_name() 
				<< " - " 
				<< cyng::io::to_str(frame)
				<< " => "
				<< receiver_.size()
				<< " receiver");

			//
			//	send data to receiver (parser)
			//
			for (auto const tsk : receiver_) {
				base_.mux_.post(tsk, 0, cyng::to_tuple(frame));
			}
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

	cyng::continuation parser_CP210x::process(std::size_t tsk, bool add)
	{
		auto pos = std::find(std::begin(receiver_), std::end(receiver_), tsk);
		if (add) {
			if (pos != std::end(receiver_)) {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> receiver task #"
					<< tsk
					<< " already inserted");
			}
			else {
				receiver_.push_back(tsk);

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> receiver task #"
					<< tsk
					<< " added - "
					<< receiver_.size()
					<< " in total");
			}
		}
		else {
			if (pos != std::end(receiver_)) {

				receiver_.erase(pos);

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> receiver task #"
					<< tsk
					<< " removed- "
					<< receiver_.size()
					<< " in total");
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> cannot remove receiver task #"
					<< tsk
					<< " - not found");
			}
		}

		//
		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}


}

