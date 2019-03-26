/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "multiple.h"
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	multiple::multiple(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::filesystem::path out
		, std::string prefix
		, std::string suffix
		, std::chrono::seconds period)
	: base_(*btp)
		, logger_(logger)
		, out_(out)
		, prefix_(prefix)
		, suffix_(suffix)
		, period_(period)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation multiple::run()
	{	
		//
		//	(re)start timer
		//
		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void multiple::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	cyng::continuation multiple::process(std::string table
		, std::size_t idx
		, std::chrono::system_clock::time_point	tp
		, boost::uuids::uuid tag
		, std::string name
		, std::string evt
		, std::string descr)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> idx: "
			<< idx);
		
		//
		//	build file name
		//
		std::stringstream ss;
		ss
			<< table
			<< '-'
			<< name 
			<< "."
			<< suffix_
			;
		
		auto const p = out_ / ss.str();
		
		//
		//	test file size and create a backup file if required
		//
		test_file_size(p);
		
		//
		//	write log data
		//
		std::ofstream ofs(p.string(), std::ios::app | std::ios::out);
		if (ofs.is_open()) {
			ofs
				<< name
				<< ';'
				<< idx
				<< ';'
				<< cyng::to_str_iso(tp)
				<< ';'
				<< tag
				<< ';'
				<< '"'
				<< evt
				<< '"'
				<< ';'
				<< '"'
				<< descr
				<< '"'
				<< std::endl;			
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void multiple::test_file_size(boost::filesystem::path p)
	{
		boost::system::error_code ec;
		const auto fs = boost::filesystem::file_size(p.string(), ec);
		if (!ec && (fs > 0x2000000))
		{	//	32 MB

			//
			//	create backup file
			//
			create_backup_file(p);

		}
	}

	void multiple::create_backup_file(boost::filesystem::path p)
	{
		std::pair<std::time_t, double> r = cyng::chrono::to_dbl_time_point(std::chrono::system_clock::now());
		std::tm tm = cyng::chrono::convert_utc(r.first);

		//	build a filename for backup file
		std::stringstream ss;
		ss
			<< cyng::chrono::year(tm)
			<< 'T'
			<< std::setw(3)
			<< std::setfill('0')
			<< cyng::chrono::day_of_year(tm)
			<< '_'
			<< cyng::chrono::time_of_day(tm)	// in seconds
			<< '_'
			<< p.string()
			;

		boost::filesystem::rename(p, ss.str());
	}

}
