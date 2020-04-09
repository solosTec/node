/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_to_xml_consumer.h"
#include "../message_ids.h"
#include <smf/sml/defs.h>
#include <cyng/async/task/base_task.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	sml_xml_consumer::sml_xml_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, boost::filesystem::path root_dir
		, std::string root_name
		, std::string endocing
		, std::chrono::seconds period)
	: base_(*btp)
		, logger_(logger)
		, ntid_(ntid)
		, root_dir_(root_dir)
		, root_name_(root_name)
		, endcoding_(endocing)
		, period_(period)
		, task_state_(task_state::INITIAL)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> initialized");
	}

	cyng::continuation sml_xml_consumer::run()
	{
		switch (task_state_) {
		case task_state::INITIAL:
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
			//	register as SML:XML consumer 
			//
			register_consumer();
			task_state_ = task_state::REGISTERED;
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

	void sml_xml_consumer::stop(bool shutdown)
	{
		//
		//	remove all open lines
		//
		lines_.clear();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_xml_consumer::process(std::uint64_t line
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

		//
		//	establish an new exporter
		//
		lines_.emplace(std::piecewise_construct,
			std::forward_as_tuple(line),
			std::forward_as_tuple(endcoding_
				, root_name_
				, (std::uint32_t)((line & 0xFFFFFFFF00000000LL) >> 32)
				, (std::uint32_t)(line & 0xFFFFFFFFLL)
				, target));

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> has "
			<< lines_.size()
			<< " active lines");

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_xml_consumer::process(std::uint64_t line
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
			<< sml::messages::name_from_value(code));

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
	cyng::continuation sml_xml_consumer::process(std::uint64_t line)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);

		auto pos = lines_.find(line);
		if (pos != lines_.end()) {

			auto filename = root_dir_ / pos->second.get_filename();

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " write "
				<< line
				<< " => "
				<< filename);

			try {
				//
				//	write XML file
				//
				if (!pos->second.write(filename)) {
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< " XML file already exist "
						<< filename);
				}
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
			}

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
	cyng::continuation sml_xml_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_xml_consumer::register_consumer()
	{
		base_.mux_.post(ntid_, STORE_EVENT_REGISTER_CONSUMER, cyng::tuple_factory("SML:XML", base_.get_id()));
	}

}
