/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage.h>
#include <storage_functions.h>

#include <cyng/sql/sql.hpp>
#include <cyng/db/details/statement_interface.h>
#include <cyng/task/controller.h>

namespace smf {

	storage::storage(cyng::db::session db)
		: db_(db)
	{}

	bool storage::cfg_insert(cyng::object const& key, cyng::object const& obj) {

		//
		//	start transaction
		//
		cyng::db::transaction	trx(db_);

		//
		//	prepare statement
		//
		auto const m = get_table_cfg();
		auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

		auto stmt = db_.create_statement();
		std::pair<int, bool> const r = stmt->prepare(sql);
		return (r.second)
			? insert_config_record(stmt, key, obj)
			: false
			;
	}

	bool storage::cfg_update(cyng::object const& key, cyng::object const& obj) {

		//
		//	start transaction
		//
		cyng::db::transaction	trx(db_);

		//
		//	prepare statement
		//
		auto const m = get_table_cfg();
		//auto const sql = cyng::sql::update(db_.get_dialect(), m).set_placeholder(m)();
		auto const sql = "UPDATE TCfg SET val = ? WHERE path = ?";

		auto stmt = db_.create_statement();
		std::pair<int, bool> const r = stmt->prepare(sql);
		return (r.second)
			? update_config_record(stmt, key, obj)
			: false
			;
	}

	bool storage::cfg_remove(cyng::object const& key) {
		//
		//	start transaction
		//
		cyng::db::transaction	trx(db_);

		//
		//	prepare statement
		//
		auto const m = get_table_cfg();
		//auto const sql = cyng::sql::remove(db_.get_dialect(), m)()
		auto const sql = "DELETE FROM TCfg WHERE path = ?";

		auto stmt = db_.create_statement();
		std::pair<int, bool> const r = stmt->prepare(sql);
		return (r.second)
			? remove_config_record(stmt, key)
			: false
			;
	}

	void storage::generate_op_log(std::uint64_t status
		, std::uint32_t evt
		, cyng::obis peer
		, cyng::buffer_t srv
		, std::string target
		, std::uint8_t nr
		, std::string details)
	{
		//
		//	start transaction
		//
		cyng::db::transaction	trx(db_);

		//
		//	prepare statement
		//
		auto const m = get_table_oplog();
		auto const sql = cyng::sql::insert(db_.get_dialect(), m).bind_values(m)();

		auto stmt = db_.create_statement();
		std::pair<int, bool> const r = stmt->prepare(sql);

		if (r.second) {
			//	skip ROWID
			stmt->push(cyng::make_object(1u), 0);	//	gen
			stmt->push(cyng::make_object(0u), 0);	//	actTime
			stmt->push(cyng::make_object(std::chrono::system_clock::now()), 0);	//	age
			stmt->push(cyng::make_object(0u), 0);	//	regPeriod
			stmt->push(cyng::make_object(0u), 0);	//	valTime
			stmt->push(cyng::make_object(status), 0);	//	status
			stmt->push(cyng::make_object(evt), 0);	//	evt
			stmt->push(cyng::make_object(peer), 13);	//	peer
			stmt->push(cyng::make_object(std::chrono::system_clock::now()), 0);	//	utc
			stmt->push(cyng::make_object(std::move(srv)), 23);	//	serverId
			stmt->push(cyng::make_object(target), 64);	//	target
			stmt->push(cyng::make_object(nr), 0);	//	pushNr
			stmt->push(cyng::make_object(details), 128);	//	details

			stmt->execute();
		}
	}


}