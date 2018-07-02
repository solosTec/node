/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_binary.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/chrono.h>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

namespace node
{

	storage_binary::storage_binary(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::string root_dir
		, std::string prefix
		, std::string suffix)
	: base_(*btp)
		, logger_(logger)
		, root_dir_(root_dir)
		, prefix_(prefix)
		, suffix_(suffix)
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

	void storage_binary::run()
	{
		CYNG_LOG_INFO(logger_, "storage_binary is running");
	}

	void storage_binary::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_binary::process(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::buffer_t const& data)
	{
		auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm time = cyng::chrono::convert_utc(tt);

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
}
