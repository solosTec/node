/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/writer.h>

#include <cyng/task/channel.h>
#include <cyng/log/record.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/uuid/nil_generator.hpp>
#include <boost/predef.h>

#ifdef _DEBUG_BROKER_WMBUS
#include <cyng/io/hex_dump.hpp>
#endif


namespace smf {

	writer::writer(cyng::channel_weak wp
		, cyng::logger logger
		, std::filesystem::path out)
	: sigs_{
			std::bind(&writer::open, this, std::placeholders::_1),
			std::bind(&writer::store, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&writer::commit, this),
			std::bind(&writer::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, logger_(logger)
		, out_(out)
		, ostream_()
	{
		auto sp = channel_.lock();
		if (sp) {
			std::size_t idx{ 0 };
			sp->set_channel_name("open", idx++);
			sp->set_channel_name("store", idx++);
			sp->set_channel_name("commit", idx++);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
		}
	}

	void writer::stop(cyng::eod)
	{	}

	void writer::open(std::string meter) {

		auto const now = std::chrono::system_clock::now();
		std::time_t const tt = std::chrono::system_clock::to_time_t(now);
		struct tm tm = { 0 };
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
		::gmtime_s(&tm, &tt);
#else
		::gmtime_r(&tt, &tm);
#endif

		//
		//	make a file name
		//
		std::stringstream ss;
		ss
			<< std::put_time(&tm, "%FT%H-%M_")
			<< meter 
			<< ".csv"
			;

		std::filesystem::path path = out_ / ss.str();

		if (ostream_.is_open())	ostream_.close();
		ostream_.open(path.string());
		if (ostream_.is_open()) {
			CYNG_LOG_INFO(logger_, "[writer] open " << path);
			ostream_ << "obis,value,unit" << std::endl;
		}
		else {
			CYNG_LOG_WARNING(logger_, "[writer] open " << path << " failed");
		}


		//auto sp = channel_.lock();
		//if (sp)	sp->suspend(interval, "start", cyng::make_tuple(address, service, interval));
	}

	void writer::store(cyng::obis code, std::string value, std::string unit) {
		CYNG_LOG_INFO(logger_, "[writer] store " << value);
		ostream_
			<< code
			<< ','
			<< value
			<< ','
			<< unit
			<< std::endl;
	}

	void writer::commit() {
		CYNG_LOG_INFO(logger_, "[writer] commit");
		if (ostream_.is_open())	ostream_.close();
	}

}


