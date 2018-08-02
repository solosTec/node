/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "sml_to_csv_consumer.h"
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

	sml_csv_consumer::sml_csv_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, boost::filesystem::path root_dir
		, std::string prefix
		, std::string suffix
		, bool header
		, std::chrono::seconds period)
	: base_(*btp)
		, logger_(logger)
		, ntid_(ntid)
		, root_dir_(root_dir)
		, prefix_(prefix)
		, suffix_(suffix)
		, header_(header)
		, period_(period)
		, task_state_(TASK_STATE_INITIAL)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

	}

	cyng::continuation sml_csv_consumer::run()
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
			//	register as SML:CSVconsumer 
			//
			register_consumer();
			task_state_ = TASK_STATE_REGISTERED;
			break;
		default:
			//CYNG_LOG_TRACE(logger_, base_.get_class_name()
			//	<< " processed "
			//	<< msg_counter_
			//	<< " messages");
			break;
		}

		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_csv_consumer::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_csv_consumer::process(std::uint64_t line
		, std::string target)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " create line "
			<< line
			<< ':'
			<< target);

		try {

			lines_.emplace(std::piecewise_construct,
				std::forward_as_tuple(line),
				std::forward_as_tuple(root_dir_
					, prefix_
					, suffix_
					, header_
					, (std::uint32_t)((line & 0xFFFFFFFF00000000LL) >> 32)
					, (std::uint32_t)(line & 0xFFFFFFFFLL)
					, target));
		}
		catch (std::exception const& ex) {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " line "
				<< line
				<< " error: "
				<< ex.what());

			return cyng::continuation::TASK_STOP;

		}

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_csv_consumer::process(std::uint64_t line
		, std::uint16_t code
		, std::size_t idx
		, cyng::tuple_t msg)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " line "
			<< line
			<< " received #"
			<< idx
			<< ':'
			<< sml::messages::name(code));

		auto pos = lines_.find(line);
		if (pos != lines_.end()) {

			pos->second.write(msg, idx);
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

	cyng::continuation sml_csv_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);

		auto size = lines_.erase(line);

		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_csv_consumer::cb(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::vector_t&& prg)
	{
		CYNG_LOG_DEBUG(logger_, "csv processor "
			<< channel
			<< ':'
			<< source
			<< ':'
			<< target
			<< " - "
			<< protocol
			<< " - "
			<< prg.size()
			<< " instructions");

	}

	void sml_csv_consumer::register_consumer()
	{
		base_.mux_.post(ntid_, 10, cyng::tuple_factory("SML:CSV", base_.get_id()));
	}

}
