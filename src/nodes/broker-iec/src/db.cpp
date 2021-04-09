/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/parse/string.h>
#include <cyng/parse/duration.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

	db::db(cyng::store& cache
		, cyng::logger logger
		, boost::uuids::uuid tag
		, cyng::channel_weak channel)
	: cache_(cache)
		, logger_(logger)
		, channel_(channel)
		, store_map_()
	{}

	void db::init(cyng::slot_ptr slot)
	{
		//
		//	initialize cache
		//
		auto const vec = get_store_meta_data();
		for (auto const m : vec) {
			CYNG_LOG_INFO(logger_, "init table [" << m.get_name() << "]");
			store_map_.emplace(m.get_name(), m);
			cache_.create_table(m);
			cache_.connect(m.get_name(), slot);
		}
		BOOST_ASSERT(vec.size() == cache_.size());
	}

	void db::disconnect(cyng::slot_ptr slot) {
		cache_.disconnect(slot);
	}

	void db::loop(std::function<void(cyng::meta_store const&)> cb) {
		for (auto const& m : store_map_) {
			cb(m.second);
		}
	}

	cyng::meta_store const& db::get_meta_iec() const {
		BOOST_ASSERT(store_map_.find("meterIEC") != store_map_.end());
		return store_map_.find("meterIEC")->second;
	}


	void db::res_insert(std::string table_name
		, cyng::key_t  key
		, cyng::data_t  data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		//CYNG_LOG_TRACE(logger_, "[cluster] db.res.insert: "
		//	<< table_name
		//	<< " - "
		//	<< data);

		cache_.insert(table_name, key, data, gen, tag);
	}

	void db::res_update(std::string table_name
		, cyng::key_t key
		, cyng::attr_t attr
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] db.res.update: "
			<< table_name
			<< " - "
			<< attr.first
			<< " => "
			<< attr.second);

		//
		//	notify all subscriptions
		// 
		cache_.modify(table_name
			, key
			, std::move(attr)
			, tag);
	}

	void db::res_remove(std::string table_name
		, cyng::key_t  key
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] db.res.remove: "
			<< table_name
			<< " - "
			<< key);

		//
		//	remove from cache
		//
		cache_.erase(table_name, key, tag);
	}

	void db::res_clear(std::string table_name
		, boost::uuids::uuid tag) {

		//
		//	clear table
		//
		cache_.clear(table_name, tag);

	}

	bool db::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, cyng::data_t const& data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[db] insert: "
			<< tbl->meta().get_name());

		return true;
	}

	/**
	 * update
	 */
	bool db::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[db] update: "
			<< tbl->meta().get_name());

		//
		//	update tasks/clients
		//

		return true;
	}

	/**
	 * remove
	 */
	bool db::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[db] remove: "
			<< tbl->meta().get_name());

		//
		//	stop tasks/clients
		//

		return true;
	}

	/**
	 * clear
	 */
	bool db::forward(cyng::table const* tbl
		, boost::uuids::uuid) {

		CYNG_LOG_TRACE(logger_, "[db] clear: "
			<< tbl->meta().get_name()
			<< " #"
			<< tbl->size());

		//
		//	stop all tasks/clients
		//

		return true;
	}

	/**
	 * transaction
	 */
	bool db::forward(cyng::table const* tbl
		, bool trx) {

		CYNG_LOG_TRACE(logger_, "[db] trx: "
			<< (trx ? " start" : " commit"));
		return true;
	}

	std::vector< cyng::meta_store > get_store_meta_data() {
		return {
			config::get_store_meter(),
			config::get_store_meterIEC()
			//config::get_config(),	//	"config"

		};
	}

}


