/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_to_abl_consumer.h"
#include <NODE_project_info.h>
#include "../message_ids.h"

#include <smf/sml/defs.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{

	sml_abl_consumer::sml_abl_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, boost::filesystem::path root_dir
		, std::string prefix
		, std::string suffix
		, std::chrono::seconds period
		, bool eol
		, cyng::object obj)
	: base_(*btp)
		, logger_(logger)
		, ntid_(ntid)
		, root_dir_(root_dir)
		, prefix_(prefix)
		, suffix_(suffix)
		, period_(period)
		, eol_(eol)
		, version_(cyng::value_cast(obj, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)))
		, task_state_(TASK_STATE_INITIAL)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< root_dir_);
	}

	cyng::continuation sml_abl_consumer::run()
	{
		switch (task_state_) {
		case TASK_STATE_INITIAL:
			if (!boost::filesystem::exists(root_dir_)) {

				CYNG_LOG_FATAL(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> out path "
					<< root_dir_
					<< " does not exists");

				return cyng::continuation::TASK_STOP;
			}

			//
			//	register as SML:ABL consumer 
			//
			register_consumer();
			task_state_ = TASK_STATE_REGISTERED;
			break;

		default:
			break;
		}

		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}


	void sml_abl_consumer::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_abl_consumer::process(std::uint64_t line
		, std::string target)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " create line "
			<< line
			//<< (std::uint32_t)((line & 0xFFFFFFFF00000000LL) >> 32)
			//<< ':'
			//<< (std::uint32_t)(line & 0xFFFFFFFFLL)
			<< ':'
			<< target);

		lines_.emplace(std::piecewise_construct,
			std::forward_as_tuple(line),
			std::forward_as_tuple(root_dir_
				, prefix_
				, suffix_
				, eol_
				, (std::uint32_t)((line & 0xFFFFFFFF00000000LL) >> 32)
				, (std::uint32_t)(line & 0xFFFFFFFFLL)
				, target));

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_abl_consumer::process(std::uint64_t line
		, std::uint16_t code
		, cyng::tuple_t msg)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " line "
			<< line
			<< " received: "
			<< sml::messages::name(code));

		auto pos = lines_.find(line);
		if (pos != lines_.end()) {

			pos->second.read(msg);
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " line "
				<< line
				<< " not found");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	 CONSUMER_REMOVE_LINE
	cyng::continuation sml_abl_consumer::process(std::uint64_t line)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);

		auto pos = lines_.find(line);
		if (pos != lines_.end()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " remove ABL line "
				<< line);

			//
			//	remove this line
			//
			lines_.erase(pos);
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " line "
				<< line
				<< " not found");
		}

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> has "
			<< lines_.size()
			<< " active lines");

		return cyng::continuation::TASK_CONTINUE;
	}


	//	EOM
	cyng::continuation sml_abl_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);
		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_abl_consumer::register_consumer()
	{
		base_.mux_.post(ntid_, STORE_EVENT_REGISTER_CONSUMER, cyng::tuple_factory("SML:ABL", base_.get_id()));
	}

}
