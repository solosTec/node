/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <notifier.h>
#include <http_server.h>
#include <db.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>

#include <iostream>

namespace smf {

	notifier::notifier(db& data
		, http_server& srv
		, cyng::logger logger)
	: db_(data)
		, http_server_(srv) 
		, logger_(logger)
	{}

	bool notifier::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, cyng::data_t const& data
		, std::uint64_t gen
		, boost::uuids::uuid) {

		//	insert
		//
		//	notify all subscriptions
		// 
		auto const r = db_.by_table(tbl->meta().get_name());
		if (!r.empty()) {

			auto range = db_.subscriptions_.equal_range(r.channel_);
			auto const count = std::distance(range.first, range.second);
			CYNG_LOG_TRACE(logger_, "update channel (insert): " << r.channel_ << " of " << count << " ws");
			for (auto pos = range.first; pos != range.second; ++pos) {
				cyng::record rec(tbl->meta(), key, data, gen);
				http_server_.notify_insert(r.channel_, cyng::record(tbl->meta(), key, data, gen), pos->second);
			}
		}

		return true;
	}

	bool notifier::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		//	update
		//
		//	notify all subscriptions
		// 
		auto const r = db_.by_table(tbl->meta().get_name());
		if (!r.empty()) {

			auto range = db_.subscriptions_.equal_range(r.channel_);
			auto const count = std::distance(range.first, range.second);
			//	get column name
			auto const col_name = tbl->meta().get_column(attr.first).name_;
			CYNG_LOG_INFO(logger_, "[channel] "
				<< r.channel_
				<< " update (#"
				<< count
				<< "): " 
				<< col_name
				<< " => "
				<< attr.second);
			auto const param = cyng::param_t(col_name, attr.second);
			for (auto pos = range.first; pos != range.second; ++pos) {
				http_server_.notify_update(r.channel_, key, param, pos->second);
			}
		}
		return true;
	}

	bool notifier::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, boost::uuids::uuid tag) {
		//	remove

		//
		//	notify all subscriptions
		// 
		auto const r = db_.by_table(tbl->meta().get_name());
		if (!r.empty()) {

			auto range = db_.subscriptions_.equal_range(r.channel_);
			auto const count = std::distance(range.first, range.second);
			CYNG_LOG_TRACE(logger_, "update channel (delete): " << r.channel_ << " of " << count << " ws");
			for (auto pos = range.first; pos != range.second; ++pos) {
				http_server_.notify_remove(r.channel_, key, pos->second);
			}
		}

		return true;
	}

	bool notifier::forward(cyng::table const* tbl
		, boost::uuids::uuid) {
		//	clear

		//
		//	notify all subscriptions
		// 
		auto const r = db_.by_table(tbl->meta().get_name());
		if (!r.empty()) {

			auto range = db_.subscriptions_.equal_range(r.channel_);
			auto const count = std::distance(range.first, range.second);
			CYNG_LOG_TRACE(logger_, "update channel (clear): " << r.channel_ << " of " << count << " ws");
			for (auto pos = range.first; pos != range.second; ++pos) {
				http_server_.notify_clear(r.channel_, pos->second);
			}
		}

		return true;
	}

	/**
	 * transaction
	 */
	bool notifier::forward(cyng::table const*
		, bool) {
		return true;
	}



}


