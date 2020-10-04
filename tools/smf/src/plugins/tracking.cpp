/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "tracking.h"
#include "../cli.h"

#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/csv.h>
#include <cyng/util/split.h>
#include <cyng/parser/chrono_parser.h>
#include <cyng/parser/buffer_parser.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/set_cast.h>
#include <cyng/chrono.h>

#include <fstream>
#include <boost/algorithm/string.hpp>

namespace node
{
	tracking::tracking(cli* cp)
		: cli_(*cp)
		, status_(status::INITIAL)
		, data_()
		, af_(cyng::filesystem::current_path() / "project-tracking.csv")
	{		
		cli_.vm_.register_function("tracking", 1, std::bind(&tracking::cmd, this, std::placeholders::_1));
	}

	tracking::~tracking()
	{}

	void tracking::cmd(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		//std::cout <<
		//	"TRACKING " << cyng::io::to_str(frame) << std::endl;

		auto const cmd = cyng::value_cast<std::string>(frame.at(0), "list");

		if (boost::algorithm::equals(cmd, "list")) {
			list();
		}
		else if (boost::algorithm::equals(cmd, "init")) {
			init(get_filename(1, frame));
		}
		else if (boost::algorithm::equals(cmd, "status")) {
			status();
		}
		else if (boost::algorithm::equals(cmd, "ammed")) {

			//
			//	modify current or last entry
			//
		}
		else if (boost::algorithm::equals(cmd, "load")) {
			load(get_filename(1, frame));
		}
		else if (boost::algorithm::equals(cmd, "flush")) {
			flush(get_filename(1, frame));
		}
		else if (boost::algorithm::equals(cmd, "start")) {

			//
			//	collect all parameters
			//
			std::string str;
			std::size_t counter{ 0 };
			for (auto obj : frame) {
				switch (counter) {
				case 0:
					break;
				case 1:
					str += cyng::io::to_str(obj);
					break;
				default:
					str += ' ';
					str += cyng::io::to_str(obj);
					break;
				}
				++counter;
			}

			start(str);
		}
		else if (boost::algorithm::equals(cmd, "stop")) {
			stop();
		}
		else if (boost::algorithm::equals(cmd, "report")) {
			if (status::OPEN == status_) {

				if (frame.size() > 1) {
					auto const type = get_parameter(1, frame, "monthly");
					if (boost::algorithm::equals(type, "monthly")) {
						report_monthly();
					}
					else if (boost::algorithm::equals(type, "daily")) {
						report_daily();
					}
					else {
						cli_.out_
							<< "unknown report type "
							<< type
							<< std::endl
							;
					}
				}
				else {
					report_monthly();
				}
			}
			else {
				cli_.out_
					<< "there is no open file "
					<< std::endl
					;
			}
		}
		else if (boost::algorithm::equals(cmd, "help")) {
			cli_.out_
				<< "help\tprint this page" << std::endl
				<< "init\t\tinitialize tracking file" << std::endl
				<< "\t> init file[.csv]" << std::endl
				<< "list\t\tlist all entries" << std::endl
				<< "report\t\tgenerate report" << std::endl
				<< "\t> report daily" << std::endl
				<< "\t> report monthly" << std::endl
				<< "status\t\tshow status" << std::endl
				<< "ammed\t\tchange current or last entry" << std::endl
				<< "load [file]\tload the specified file" << std::endl
				<< "flush\t\tsave current state to file" << std::endl
				<< "start [comment]\tstart a new entry" << std::endl
				<< "stop\t\tstop running item" << std::endl
				;
		}
		else {
			cli_.out_ 
				<< "TRACKING - unknown command: " 
				<< cmd 
				<< std::endl;
		}
	}

	std::string tracking::get_status() const
	{
		switch (status_) {
		case status::INITIAL:	return "CLOSED";
		case status::OPEN:		return "OPEN";
		case status::RUNNING:	return "RUNNING";
		default:
			break;
		}
		return "ERROR";
	}

	void tracking::status()
	{
		cli_.out_
			<< get_status()
			<< std::endl;

		if (status_ == status::RUNNING) {
			if (data_.empty()) {
				cli_.out_
					<< "***error: no data set found"
					<< std::endl;
			}
			else {
				auto & current = data_.back();
				cyng::tuple_t tpl = cyng::to_tuple(current);
				if (tpl.size() == 3) {

					//
					//	current entry
					//
					auto const now = std::chrono::system_clock::now();
					auto pos = tpl.begin();	//	start time
					auto const start = cyng::value_cast(*pos, now);

					//
					//	update
					//
					std::advance(pos, 1);
					auto const duration = std::chrono::duration_cast<std::chrono::seconds>(now - start);
					*pos = cyng::make_object(duration);

					current = cyng::make_object(tpl);

					cli_.out_
						<< cyng::io::to_str(current)
						<< std::endl;

				}
			}
		}
		else if (status_ == status::OPEN) {
			cli_.out_
				<< "["
				<< af_
				<< "] is loaded with "
				<< data_.size()
				<< " entries"
				<< std::endl;
		}
	}

	void tracking::init(std::string filename)
	{
		auto const p = get_path(filename);
		if (!cyng::filesystem::exists(p)) {

			std::ofstream fout(p.string(), std::ios::trunc);
			if (fout.is_open()) {
				//fout
				//	<< "\"start\",\"duration\",\"msg\""
				//	<< std::endl
				//	;
				af_ = p;
			}
			else {
				cli_.out_
					<< "***error: cannot open "
					<< filename
					<< std::endl;
			}
		}
		else {
			cli_.out_
				<< "***error: "
				<< filename
				<< " already exists"
				<< std::endl;
		}
	}

	void tracking::load(std::string filename)
	{
		if (status::RUNNING == status_) {
			cli_.out_
				<< "***warning: current state is "
				<< get_status()
				<< " - stop running item or data get lost"
				<< std::endl;
			return;
		}

		auto const p = get_path(filename);

		if (cyng::filesystem::exists(p) && cyng::filesystem::is_regular_file(p)) {
			status_ = status::OPEN;
			af_ = p;
			data_ = cyng::csv::read_file(p.string());

			//
			//	convert string => duration
			//
			std::chrono::seconds total_duration(0);
			for (auto& rec : data_) {
				cyng::tuple_t tpl = cyng::to_tuple(rec);
				if (tpl.size() == 3) {
					auto pos = tpl.begin();
					++pos;
					auto s = cyng::value_cast<std::string>(*pos, "");
					auto const r = cyng::parse_timespan_seconds(s);
					if (r.second) {
						total_duration += r.first;
						*pos = cyng::make_object(r.first);
						rec = cyng::make_object(tpl);
					}
				}
			}

			cli_.out_
				<< "["
				<< p
				<< "] contains "
				<< data_.size()
				<< " entries with a total duration of "
				<< cyng::to_str(total_duration)
				<< std::endl;

		}
		else {
			cli_.out_
				<< "***error: cannot open "
				<< p
				<< std::endl;
		}
	}

	void tracking::start(std::string msg)
	{
		if (status::OPEN != status_) {
			cli_.out_
				<< "***warning: current state is "
				<< get_status()
				<< " - no project is loaded"
				<< std::endl;
			return;
		}
		data_.push_back(cyng::make_object(create_record(msg)));

		//
		//	update status
		//
		status_ = status::RUNNING;
		cli_.out_
			<< "item #"
			<< data_.size()
			<< " is running"
			<< std::endl;
	}

	void tracking::stop()
	{
		if (!data_.empty()) {

			//
			//	get current entry
			//
			auto & current = data_.back();
			cyng::tuple_t tpl = cyng::to_tuple(current);

			if (tpl.size() == 3) {

				//
				//	current entry
				//
				auto const now = std::chrono::system_clock::now();
				auto pos = tpl.begin();	//	start time

				//
				//	get start time
				//
				auto const start = cyng::value_cast(*pos, now);

				//
				//	update duration
				//
				std::advance(pos, 1);

				auto const duration = std::chrono::duration_cast<std::chrono::seconds>(now - start);
				*pos = cyng::make_object(duration);

				cli_.out_
					<< cyng::io::to_str(tpl)
					<< std::endl
					;

				//
				//	replace existing entry
				//
				current = cyng::make_object(tpl);

				//
				//	update status and flush
				//
				status_ = status::OPEN;
				this->flush(af_.string());

			}
		}
	}

	void tracking::flush(std::string filename)
	{
		if (status::OPEN == status_) {
			//
			//	make changes permanent
			//
			auto const p = get_path(filename);
			if (!cyng::csv::write_file(p.string(), data_)) {

				cli_.out_
					<< "cannot write to file "
					<< p
					<< std::endl
					;
			}
		}
		else {
			cli_.out_
				<< "there is no open file "
				<< std::endl
				;
		}
	}

	void tracking::list() const
	{
		cli_.out_
			<< "database contains "
			<< data_.size()
			<< " entries"
			<< std::endl;

		for (auto const& obj : data_) {
			auto const tpl = cyng::to_tuple(obj);
			if (tpl.size() == 3) {
				for (auto const& row : tpl) {
					cli_.out_
						<< cyng::io::to_str(row)
						<< " - ";
				}
				cli_.out_
					<< std::endl;
			}
			else {
				cli_.out_
					<< "wrong format: "
					<< cyng::io::to_str(obj)
					<< std::endl;
				break;
			}
		}
	}

	void tracking::report_monthly()
	{
		cli_.out_
			<< "=== monthly report ("
			<< data_.size()
			<< ") ==="
			<< std::endl;

		//std::chrono::seconds total_duration(0);
		//for (auto& rec : data_) {
		//	cyng::tuple_t tpl = cyng::to_tuple(rec);
		//	if (tpl.size() == 3) {
		//		auto pos = tpl.begin();
		//		++pos;
		//		auto s = cyng::value_cast<std::string>(*pos, "");
		//		auto const r = cyng::parse_timespan_seconds(s);
		//		if (r.second) {
		//			total_duration += r.first;
		//			*pos = cyng::make_object(r.first);
		//			rec = cyng::make_object(tpl);
		//		}
		//	}
		//}

	}

	void tracking::report_daily()
	{
		cli_.out_
			<< "=== daily report ("
			<< data_.size()
			<< ") ==="
			<< std::endl;

		//std::uint64_t days_since_epoch(std::chrono::system_clock::time_point);

		//
		//	store reported time for every days
		//
		std::map<std::uint64_t, std::chrono::seconds> report;

		std::chrono::seconds total_duration(0);
		for (auto& rec : data_) {
			cyng::tuple_t tpl = cyng::to_tuple(rec);
			if (tpl.size() == 3) {

				auto pos = tpl.begin();

				//
				//	1. entry: when
				//
				auto const tp = cyng::value_cast(*pos, std::chrono::system_clock::now());
				auto const days = cyng::chrono::days_since_epoch(tp);

				//
				//	2. entry: how long
				//
				std::advance(pos, 1);
				auto const sec = cyng::value_cast(*pos, std::chrono::seconds(0));

				auto const it = report.find(days);
				if (it == report.end()) {
					report.emplace(days, sec);
				}
				else {
					it->second += sec;

				}
			}
		}

		for (auto const& day : report) {

			auto const tp = cyng::chrono::init_tp(1970, 1, 1, 0, 0, 0.0) + cyng::chrono::days(day.first);
			cli_.out_
				<< cyng::date_to_str(tp)
				<< ": "
				;

			if (day.second.count() < 60) {

				cli_.out_
					<< cyng::to_str(day.second)
					<< "\t- "
					<< day.second.count()
					<< " seconds"
					<< std::endl;

			}
			else if (day.second.count() < 60 * 60 * 24) {

				cli_.out_
					<< cyng::to_str(day.second)
					<< "\t- "
					<< (day.second.count() / 60)
					<< " minutes"
					<< std::endl;

			}
			else {
				cli_.out_
					<< cyng::to_str(day.second)
					<< std::endl;
			}

		}
	}

	std::string get_parameter(std::size_t idx, cyng::vector_t const& frame, std::string def)
	{
		return (idx < frame.size())
			? cyng::value_cast<std::string>(frame.at(idx), def)
			: def
			;
	}

	std::string tracking::get_filename(std::size_t idx, cyng::vector_t const& frame)
	{
		return get_parameter(idx, frame, af_.string());
	}

	cyng::tuple_t create_record(std::string msg)
	{
		return cyng::tuple_factory(std::chrono::system_clock::now(), std::chrono::seconds(0), msg);
	}

	cyng::filesystem::path get_path(std::string filename)
	{
		cyng::filesystem::path p(filename);
		if (!p.has_extension()) {
			p.replace_extension(".csv");
		}
		return p;
	}

}