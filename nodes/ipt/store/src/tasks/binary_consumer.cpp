/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "binary_consumer.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/chrono.h>
#include <cyng/factory/set_factory.h>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

namespace node
{

	binary_consumer::binary_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, std::string root_dir
		, std::string prefix
		, std::string suffix
		, std::chrono::seconds period)
	: base_(*btp)
		, logger_(logger)
		, ntid_(ntid)
		, root_dir_(root_dir)
		, prefix_(prefix)
		, suffix_(suffix)
		, period_(period)
		, task_state_(TASK_STATE_INITIAL)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> output to "
			<< root_dir
			<< '/'
			<< prefix_
			<< "...."
			<< suffix_);

	}

	cyng::continuation binary_consumer::run()
	{
		CYNG_LOG_INFO(logger_, "binary_consumer is running");
		switch (task_state_) {
		case TASK_STATE_INITIAL:
			//
			//	register as SML:XML consumer 
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

	void binary_consumer::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation binary_consumer::process(std::uint32_t channel
		, std::uint32_t source
		, std::string protocol
		, std::string const& target
		, cyng::buffer_t const& data)
	{
		auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm time = cyng::chrono::convert_utc(tt);

		//
		//	remove ':' from protocol name
		//
		protocol.erase(std::remove(protocol.begin(), protocol.end(), ':'), protocol.end());

		//
		//	generate file name
		//
		std::stringstream ss;
		ss
			<< prefix_
			<< '-'
			<< std::hex
			<< std::setfill('0') 
			<< std::setw(4)
			<< channel
			<< '-'
			<< std::setw(4)
			<< source
			<< '-'
			<< std::dec
			<< cyng::chrono::year(time)
			<< std::setw(2)
			<< cyng::chrono::month(time)
			<< std::setw(2)
			<< 'T'
			<< cyng::chrono::day(time)
			<< std::setw(2)
			<< cyng::chrono::hour(time)
			<< std::setw(2)
			<< cyng::chrono::minute(time)
			<< '-'
			<< protocol
			<< '-'
			<< target
			<< '.'
			<< suffix_
			;

		boost::filesystem::path file = boost::filesystem::path(root_dir_) / ss.str();

		std::ofstream of(file.string(), std::ios::out | std::ios::binary | std::ios::app);
		if (of.is_open())
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> write "
				<< data.size()
				<< " bytes to "
				<< file);

			of.write(data.data(), data.size());
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> cannot open file "
				<< file);

		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void binary_consumer::register_consumer()
	{
		base_.mux_.post(ntid_, 8, cyng::tuple_factory("ALL:RAW", base_.get_id()));
	}

}
