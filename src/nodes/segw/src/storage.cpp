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

}