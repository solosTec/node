/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "single.h"
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	single::single(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::filesystem::path root
		, std::string prefix
		, std::string suffix
		, std::chrono::seconds period)
	: base_(*btp)
		, logger_(logger)
		, file_name_(root / (prefix + "." + suffix))
		, period_(period)
		, ofs_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< file_name_);
	}

	cyng::continuation single::run()
	{	
		if (ofs_.is_open()) {
			ofs_.flush();
			ofs_.close();
			test_file_size();
		}

		ofs_.open(file_name_.string(), std::ios::app | std::ios::out);
		if (ofs_.is_open()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> re-open "
				<< file_name_);
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> re-open "
				<< file_name_
				<< " failed");
		}

		//
		//
		//	(re)start timer
		//
		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void single::stop()
	{
		if (ofs_.is_open()) {
			ofs_.flush();
			ofs_.close();
		}

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	cyng::continuation single::process(std::string table
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

		if (ofs_.is_open()) {
			ofs_
				<< table
				<< ';'
				<< idx
				<< ';'
				<< cyng::to_str_iso(tp)
				<< ';'
				<< tag
				<< ';'
				<< name
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

	void single::test_file_size()
	{
		boost::system::error_code ec;
		const auto fs = boost::filesystem::file_size(file_name_.string(), ec);
		if (!ec && (fs > 0x2000000))
		{	//	32 MB

			//
			//	create backup file
			//
			create_backup_file();

		}
	}

	void single::create_backup_file()
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
			;

		const std::string tag = ss.str();
		const boost::filesystem::path backup
			= file_name_.parent_path() / ((file_name_.stem().string() + "_backup_" + tag) + file_name_.extension().string());

		boost::filesystem::rename(file_name_, backup);
	}


}
