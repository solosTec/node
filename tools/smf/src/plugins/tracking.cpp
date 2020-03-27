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

#include <fstream>
#include <boost/algorithm/string.hpp>

namespace node
{
	tracking::tracking(cli* cp)
		: cli_(*cp)
		, status_(TRACKING_INITIAL)
		, data_()
	{
		cli_.vm_.register_function("tracking", 1, std::bind(&tracking::cmd, this, std::placeholders::_1));
	}

	tracking::~tracking()
	{}

	void tracking::cmd(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		std::cout <<
			"TRACKING " << cyng::io::to_str(frame) << std::endl;

		auto const cmd = cyng::value_cast<std::string>(frame.at(0), "list");

		if (boost::algorithm::equals(cmd, "list")) {
			list();
		}
		else if (boost::algorithm::equals(cmd, "status")) {
			cli_.out_
				<< get_status()
				<< std::endl;
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
			start(get_parameter(1, frame, "ToDo"));
		}
		else if (boost::algorithm::equals(cmd, "stop")) {
			stop();
		}
		else if (boost::algorithm::equals(cmd, "help")) {
			std::cout 
				<< "help\t\tprint this page" << std::endl
				<< "list" << std::endl
				<< "status" << std::endl
				<< "ammed" << std::endl
				<< "load" << std::endl
				<< "flush" << std::endl
				<< "start" << std::endl
				<< "stop" << std::endl
				;
		}
		else {
			std::cout <<
				"TRACKING - unknown command: " 
				<< cmd 
				<< std::endl;
		}
	}

	std::string tracking::get_status() const
	{
		switch (status_) {
		case TRACKING_INITIAL:	return "CLOSED";
		case TRACKING_OPEN:	return "OPEN";
		case TRACKING_RUNNING:	return "RUNNING";
		default:
			break;
		}
		return "ERROR";
	}

	void tracking::load(std::string filename)
	{
		if (boost::filesystem::exists(filename) && boost::filesystem::is_regular_file(filename)) {
			status_ = TRACKING_OPEN;
			data_ = cyng::csv::read_file(filename);
			cli_.out_
				<< get_status()
				<< " <"
				<< filename
				<< "> contains "
				<< data_.size()
				<< " entries"
				<< std::endl;
		}
		else {
			cli_.out_
				<< "***error: cannot open "
				<< filename
				<< std::endl;

		}
	}

	void tracking::start(std::string msg)
	{
		data_.push_back(cyng::make_object(create_record(msg)));
	}

	void tracking::stop()
	{
		if (!data_.empty()) {

			//
			//	get timestamp
			//
			auto const now = std::chrono::system_clock::now();
			auto const time = get_time(now);

			//
			//	get current entry
			//
			auto current = data_.back();

			cyng::tuple_t tpl;
			tpl = cyng::value_cast(current, tpl);

			if (tpl.size() == 4) {

				//
				//	update timestamp
				//
				auto pos = tpl.begin();
				std::advance(pos, 2);

				*pos = cyng::make_object(time);

				cli_.out_
					<< cyng::io::to_str(tpl)
					<< std::endl
					;

				//
				//	replace existing entry
				//
				data_.pop_back();
				data_.push_back(cyng::make_object(tpl));

			}
		}
	}

	void tracking::flush(std::string filename)
	{
		//
		//	make changes permanent
		//
		if (!cyng::csv::write_file(filename, data_)) {

			cli_.out_
				<< "cannot write to file "
				<< filename
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
			cli_.out_
				<< cyng::io::to_str(obj)
				<< std::endl;
		}
	}

	std::string get_parameter(std::size_t idx, cyng::vector_t const& frame, std::string def)
	{
		return (idx < frame.size())
			? cyng::value_cast<std::string>(frame.at(idx), def)
			: def
			;
	}

	std::string get_filename(std::size_t idx, cyng::vector_t const& frame)
	{
		return get_parameter(idx, frame, "project-tracking.csv");
	}

	cyng::tuple_t create_record(std::string msg)
	{
		auto const now = std::chrono::system_clock::now();
		auto const date = cyng::date_to_str(now);
		auto const start = get_time(now);
		BOOST_ASSERT(start.size() == 8);
		return cyng::tuple_factory(date, start, start, msg);
	}

	std::string get_time(std::chrono::system_clock::time_point tp)
	{
		auto const time = cyng::chrono::duration_of_day(tp);
		return cyng::ts_to_str(time);	//	format hh:mm:ss
	}


}