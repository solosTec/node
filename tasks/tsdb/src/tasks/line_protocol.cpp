/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "line_protocol.h"
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	line_protocol::line_protocol(cyng::async::base_task* btp
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

	cyng::continuation line_protocol::run()
	{	
		if (ofs_.is_open()) {
			ofs_.flush();
			ofs_.close();
			test_file_size();
		}

		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> re-open "
			<< file_name_);

		ofs_.open(file_name_.string(), std::ios::app | std::ios::out);

		//
		//
		//	(re)start timer
		//
		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void line_protocol::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	cyng::continuation line_protocol::process(std::string table
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
				<< name
				<< ','
				<< "tag="
				<< tag
				<< ','
				<< "event="
				<< escape(evt)
				<< ','
				<< "value="
				<< escape(descr)
				<< ' '
				<< "index="
				<< idx
				<< ' '
				<< tp.time_since_epoch().count()
				<< std::endl;
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	void line_protocol::test_file_size()
	{
		boost::system::error_code ec;
		const auto fs = boost::filesystem::file_size(file_name_.string());
		if (!ec && (fs > 0x800000))
		{	//	8 MB

			//
			//	create backup file
			//
			create_backup_file();

		}
	}

	void line_protocol::create_backup_file()
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

	std::string line_protocol::escape(std::string const& str)
	{
		std::string r;
		r.reserve(str.size());
		for (auto const c : str) {
			switch (c) {
			case ' ':
			case ',':
			case '=':
			case '"':
				r.push_back('\\');
				break;
			default:
				break;
			}
			r.push_back(c);
		}
		return r;
	}


}
