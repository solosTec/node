/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/persistence.h>
#include <storage.h>

#include <cyng/log/record.h>

#include <boost/algorithm/string.hpp>

namespace smf
{
	persistence::persistence(std::weak_ptr<cyng::channel> wp
		, cyng::logger logger
		, cyng::store& cache
		, storage& s)
		: sigs_{
			std::bind(&persistence::stop, this, std::placeholders::_1),
			std::bind(&persistence::insert, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
			std::bind(&persistence::modify, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
			std::bind(&persistence::remove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&persistence::clear, this, std::placeholders::_1, std::placeholders::_2)
			//std::bind(&persistence::connect, this)
	}, channel_(wp)
		, logger_(logger)
		, cache_(cache)
		, storage_(s)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("insert", 1);
			sp->set_channel_name("modify", 2);
			sp->set_channel_name("remove", 3);
			sp->set_channel_name("clear", 4);
			//sp->set_channel_name("connect", 5);
		}

		connect();
		CYNG_LOG_INFO(logger_, "persistence ready");
	}

	void persistence::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "persistence stopped");
	}

	void persistence::connect() {

		cache_.connect("cfg", cyng::slot(channel_));
		CYNG_LOG_INFO(logger_, "persistence connected");
	}

	void persistence::insert(cyng::table const* tbl
		, cyng::key_t key
		, cyng::data_t data
		, std::uint64_t gen
		, boost::uuids::uuid) {

		CYNG_LOG_TRACE(logger_, "insert " << tbl->meta().get_name());
		if (boost::algorithm::equals(tbl->meta().get_name(), "cfg")) {
			CYNG_LOG_TRACE(logger_, key.at(0) << " := " << data.at(0));
			storage_.cfg_insert(key.at(0), data.at(0));
		}
		else {
			// use default mechanism
		}
	}

	/**
	 * signal an modify event
	 */
	void persistence::modify(cyng::table const* tbl
		, cyng::key_t key
		, cyng::attr_t attr
		, std::uint64_t
		, boost::uuids::uuid) {

		if (boost::algorithm::equals(tbl->meta().get_name(), "cfg")) {
			CYNG_LOG_TRACE(logger_, key.at(0) << " = " << attr.second);
			storage_.cfg_update(key.at(0), attr.second);
		}
		else {
			// use default mechanism
			CYNG_LOG_TRACE(logger_, "modify " << tbl->meta().get_name());
		}

	}

	/**
	 * signal an remove event
	 */
	void persistence::remove(cyng::table const* tbl
		, cyng::key_t key
		, boost::uuids::uuid) {

		CYNG_LOG_TRACE(logger_, "remove " << tbl->meta().get_name());

	}

	/**
	 * signal an clear event
	 */
	void persistence::clear(cyng::table const* tbl
		, boost::uuids::uuid) {

		CYNG_LOG_TRACE(logger_, "clear " << tbl->meta().get_name());

	}


}

