/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "storage.h"
#include "segw.h"
#include <smf/sml/obis_db.h>

#include <cyng/db/interface_statement.h>
#include <cyng/db/sql_table.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/buffer_cast.h>
#include <cyng/table/restore.h>

#include <cyng/sql.h>
#include <cyng/sql/dsl/binary_expr.hpp>
#include <cyng/sql/dsl/list_expr.hpp>
#include <cyng/sql/dsl/operators.hpp>
#include <cyng/sql/dsl/assign.hpp>
#include <cyng/sql/dsl/aggregate.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	storage::storage(boost::asio::io_service& ios, cyng::db::connection_type type)
		: pool_(ios, type)
	{}

	cyng::table::meta_map_t storage::get_meta_map()
	{
		cyng::table::meta_map_t mm;
		for (auto tbl : create_storage_meta_data()) {
			mm.emplace(tbl->get_name(), tbl);
		}
		return mm;
	}

	bool storage::start(cyng::param_map_t cfg)
	{
		return pool_.start(cfg);
	}

	cyng::db::session storage::get_session()
	{
		return pool_.get_session();
	}

	void storage::generate_op_log(std::uint64_t status
		, std::uint32_t evt
		, sml::obis peer
		, cyng::buffer_t srv
		, std::string target
		, std::uint8_t nr
		, std::string details)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd("TOpLog", s.get_dialect());
		if (!cmd.is_valid())	return;

		auto const sql = cmd.insert()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);

		//cyng::table::make_meta_table_gen<1, 10>("TOpLog",
		//	{ "ROWID"		//	index - with SQLite this prevents creating a column
		//					//	-- body
		//	, "actTime"		//	actual time
		//	, "regPeriod"	//	register period
		//	, "valTime"		//	val time
		//	, "status"		//	status word
		//	, "event"		//	event code
		//	, "peer"		//	peer address
		//	, "utc"			//	UTC time
		//	, "serverId"	//	server ID (meter)
		//	, "target"		//	target name
		//	, "pushNr"		//	operation number

		if (r.second) {

			stmt->push(cyng::make_object(1u), 0);	//	gen
			stmt->push(cyng::make_object(0u), 0);	//	actTime
			stmt->push(cyng::make_object(0u), 0);	//	regPeriod
			stmt->push(cyng::make_object(0u), 0);	//	valTime
			stmt->push(cyng::make_object(status), 0);	//	status
			stmt->push(cyng::make_object(evt), 0);	//	evt
			stmt->push(cyng::make_object(peer.to_buffer()), 13);	//	peer
			stmt->push(cyng::make_now(), 0);	//	utc
			stmt->push(cyng::make_object(srv), 23);	//	serverId
			stmt->push(cyng::make_object(target), 64);	//	target
			stmt->push(cyng::make_object(nr), 0);	//	pushNr
			stmt->push(cyng::make_object(details), 128);	//	details

			stmt->execute();
		}
	}

	void storage::loop(std::string name, loop_f f)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(name, s.get_dialect());
		if (!cmd.is_valid())	return;

		auto const sql = cmd.select().all()();
		auto stmt = s.create_statement();
		stmt->prepare(sql);

		//
		//	read all results
		//
		while (auto res = stmt->get_result()) {

			//
			//	Convert SQL result to record
			//
			auto const rec = cyng::to_record(cmd.get_meta(), res);

			//
			//	false terminates the loop
			//
			if (!f(rec))	break;
		}
	}

	void storage::loop_oplog(std::chrono::system_clock::time_point start
		, std::chrono::system_clock::time_point end
		, loop_f f)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd("TOpLog", s.get_dialect());
		if (!cmd.is_valid())	return;

		//	CREATE TABLE TOpLog (gen INT, actTime FLOAT, regPeriod INT, valTime FLOAT, status INT, event INT, peer BLOB, utc FLOAT, serverId BLOB, target TEXT, pushNr INT, details TEXT);
		//	"utc" contains the time stamp
		auto const sql = cmd.select().all().where(cyng::sql::column(9) > cyng::sql::constant(start) && cyng::sql::column(9) < cyng::sql::constant(end))();
		auto stmt = s.create_statement();
		stmt->prepare(sql);

		//
		//	read all results
		//
		while (auto res = stmt->get_result()) {

			//
			//	Convert SQL result to record
			//
			auto const rec = cyng::to_record(cmd.get_meta(), res);

			//
			//	false terminates the loop
			//
			if (!f(rec))	break;
		}
	}

	void storage::loop_profile(sml::obis profile
		, std::chrono::system_clock::time_point start
		, std::chrono::system_clock::time_point end
		, profile_f cb)
	{
		auto s = pool_.get_session();
		auto const q = profile.get_quantities();
		std::string const sql = get_sql_select_all(q);
		if (sql.empty())	return;

		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			//
			//	read all results
			//
			while (auto res = stmt->get_result()) {
				BOOST_ASSERT(res->column_count() == 9);

				auto const srv_id = cyng::to_buffer(res->get(1, cyng::TC_BUFFER, 9));
				auto const tsdix = cyng::value_cast<std::uint64_t>(res->get(2, cyng::TC_UINT64, 0), 0);
				//	3. actTime
				auto const act_time = cyng::value_cast(res->get(3, cyng::TC_TIME_POINT, 0), std::chrono::system_clock::now());
				//	4. status
				auto const status = cyng::value_cast<std::uint32_t>(res->get(4, cyng::TC_UINT32, 0), 0u);
				sml::obis const code(cyng::to_buffer(res->get(5, cyng::TC_BUFFER, 6)));
				//	6. val
				auto const val = cyng::value_cast<std::string>(res->get(6, cyng::TC_STRING, 0), "");
				//	7. type
				auto const type = cyng::value_cast<std::uint32_t>(res->get(7, cyng::TC_UINT32, 0), 0);
				auto const scaler = cyng::value_cast<std::int8_t>(res->get(8, cyng::TC_INT8, 0), 0);
				auto const unit = cyng::value_cast<std::uint8_t>(res->get(9, cyng::TC_UINT8, 0), 0);

				//
				//	callback
				//	false terminates the loop
				//
				if (!cb(srv_id
					, tsdix
					, time_index_to_time_point(q, tsdix)
					, act_time
					, status
					, code
					, scaler
					, unit
					//	restore object from string and type info
					, cyng::table::restore(val, type)))	break;
			}
		}
	}


	cyng::sql::command storage::create_cmd(std::string name, cyng::sql::dialect d)
	{
		auto pos = mm_.find(name);
		return (pos != mm_.end())
			? cyng::sql::command(pos->second, d)
			: cyng::sql::command(cyng::table::meta_table_ptr(), d)
			;
	}

	bool storage::update(std::string tbl
		, cyng::table::key_type const& key
		, std::string col
		, cyng::object obj
		, std::uint64_t gen)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		//
		//	precondition is that both tables (in memory and SQL) use the same column names
		//
		auto sql = cmd.update(cyng::sql::make_assign(col, cyng::sql::make_placeholder())).by_key()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			stmt->push(obj, 0);
			for (auto idx = 0; idx < key.size(); ++idx) {
				stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
			}

			if (!stmt->execute()) {
				return false;
			}

			stmt->clear();

			//
			//	update gen(eration)
			//
			auto sql = cmd.update(cyng::sql::make_assign("gen", cyng::sql::make_placeholder())).by_key()();
			r = stmt->prepare(sql);
			if (r.second) {
				//	UPDATE TCfg SET gen = ? WHERE (path = ?)
				stmt->push(cyng::make_object(gen), 0);
				for (auto idx = 0; idx < key.size(); ++idx) {
					stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
				}

				if (!stmt->execute()) {
					return false;
				}

				stmt->clear();
			}
			return true;
		}
		//
		//	syntax error
		//

		return false;
	}

	bool storage::update(std::string tbl
		, cyng::table::key_type const& key
		, cyng::param_t const& param
		, std::uint64_t gen)
	{
		return update(tbl
			, key
			, param.first
			, param.second
			, gen);
	}

	bool storage::update(std::string tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& body
		, std::uint64_t gen)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		//
		//	precondition is that both tables (in memory and SQL) use the same column names
		//	sql_update update();
		//

		auto sql = cmd.update().by_key()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			//
			//	table data 
			//	UPDATE table SET col1 = x1, col2 = x2) WHERE key0 = x0;
			//
			auto meta = cmd.get_meta();

			auto idx = 0;
			meta->loop_body([&](cyng::table::column&& col) {
				if (idx == 0) {
					stmt->push(cyng::make_object(gen), 0);
				}
				else {
					stmt->push(body.at(idx - 1), col.width_);
				}
				++idx;
			});

			//
			//	table key
			//
			for (auto idx = 0; idx < key.size(); ++idx) {
				stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
			}

			if (!stmt->execute()) {
				return false;
			}

			stmt->clear();

			//
			//	update gen(eration)
			//
			auto sql = cmd.update(cyng::sql::make_assign("gen", cyng::sql::make_placeholder())).by_key()();
			r = stmt->prepare(sql);
			if (r.second) {
				//	UPDATE TCfg SET gen = ? WHERE (path = ?)
				stmt->push(cyng::make_object(gen), 0);
				for (auto idx = 0; idx < key.size(); ++idx) {
					stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
				}

				if (!stmt->execute()) {
					return false;
				}

				stmt->clear();
				return true;
			}
		}

		return false;
	}

	bool storage::insert(std::string tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& body
		, std::uint64_t gen)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

#ifdef _DEBUG
		if (boost::algorithm::equals(tbl, "TDataCollector")) {
			if (test_for_entry(s, tbl, key)) {
				return false;
			}
		}
#endif

		auto const sql = cmd.insert()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			std::size_t idx{ 0 };
			for (auto const& obj : key) {
				stmt->push(obj, cmd.get_meta()->get_width(idx++));
			}
			stmt->push(cyng::make_object(gen), cmd.get_meta()->get_width(idx++));
			for (auto const& obj : body) {
				stmt->push(obj, cmd.get_meta()->get_width(idx++));
			}
			if (!stmt->execute())	return false;
			stmt->clear();

#ifdef _DEBUG
			if (boost::algorithm::equals(tbl, "TDataCollector")) {
				if (!test_for_entry(s, tbl, key)) {
					return false;
				}
			}
#endif
			return true;

		}

		return false;
	}

	bool storage::remove(std::string tbl
		, cyng::table::key_type const& key)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

#ifdef _DEBUG
		if (boost::algorithm::equals(tbl, "TDataCollector")) {
			if (!test_for_entry(s, tbl, key)) {
				return false;
			}
		}
#endif

		auto const sql = cmd.remove().by_key()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			BOOST_ASSERT(r.first == key.size());

			for (auto idx = 0u; idx < key.size(); ++idx) {
				stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
			}
			if (!stmt->execute())
			{
				return false;
			}
			stmt->clear();

#ifdef _DEBUG
			if (boost::algorithm::equals(tbl, "TDataCollector")) {
				if (test_for_entry(s, tbl, key)) {
					return false;
				}
			}
#endif

			return true;
		}

		return false;
	}

#ifdef _DEBUG
	bool storage::test_for_entry(cyng::db::session s, std::string tbl, cyng::table::key_type const& key)
	{
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		auto const sql = cmd.select().count().by_key()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			BOOST_ASSERT(r.first == key.size());

			for (auto idx = 0u; idx < key.size(); ++idx) {
				stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
			}
			auto res = stmt->get_result();
			if (res) {
				auto const obj = res->get(1, cyng::TC_UINT64, 0);
				auto const count = cyng::numeric_cast<std::uint64_t>(obj, 0);
				return count != 0;
			}
		}

		return false;
	}
#endif

	std::uint64_t storage::count(std::string tbl)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		auto const sql = cmd.select().count()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			auto res = stmt->get_result();
			if (res) {
				auto const obj = res->get(1, cyng::TC_UINT64, 0);
				auto const count = cyng::numeric_cast<std::uint64_t>(obj, 0);
				return count;
			}
		}

		return 0u;
	}

	bool storage::exists(std::string tbl
		, cyng::table::key_type const& key)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		auto const sql = cmd.select().count().by_key()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			BOOST_ASSERT(r.first == key.size());

			for (auto idx = 0u; idx < key.size(); ++idx) {
				stmt->push(key.at(idx), cmd.get_meta()->get_width(idx));
			}
			auto res = stmt->get_result();
			if (res) {
				auto const obj = res->get(1, cyng::TC_UINT64, 0);
				auto const count = cyng::numeric_cast<std::uint64_t>(obj, 0);
				return count != 0;
			}
		}

		return false;
	}

	void storage::merge_profile_meta_8181C78610FF(cyng::buffer_t srv_id
		, std::uint64_t minutes
		, std::chrono::system_clock::time_point ts
		, std::uint32_t status)
	{
		auto const key = cyng::table::key_generator(srv_id, minutes + 1);
		auto const b = exists("TProfile_8181C78610FF", key);
		if (b) {
			//
			//	update
			//
		}
		else {
			//
			//	insert
			//
			insert("TProfile_8181C78610FF"
				, key
				, cyng::table::data_generator(ts, 0, status)
				, 1);
		}
	}

	void storage::merge_profile_meta_8181C78611FF(cyng::buffer_t srv_id
		, std::uint64_t quarters	//	quarter hours
		, std::chrono::system_clock::time_point ts
		, std::uint32_t status)
	{
		auto const key = cyng::table::key_generator(srv_id, quarters + 1);
		auto const b = exists("TProfile_8181C78611FF", key);
		if (b) {
			//
			//	update
			//
		}
		else {
			//
			//	insert
			//
			insert("TProfile_8181C78611FF"
				, key
				, cyng::table::data_generator(ts, 0, status)
				, 1);
		}
	}

	void storage::merge_profile_meta_8181C78612FF(cyng::buffer_t srv_id
		, std::uint64_t hours
		, std::chrono::system_clock::time_point ts
		, std::uint32_t status)
	{	//	60 min profile meta data
		auto const key = cyng::table::key_generator(srv_id, hours + 1);
		auto const b = exists("TProfile_8181C78612FF", key);
		if (b) {
			//
			//	update
			//
			update("TProfile_8181C78612FF"
				, key
				, cyng::table::data_generator(ts, 0, status)
				, 2);
		}
		else {
			//
			//	insert
			//
			insert("TProfile_8181C78612FF"
				, key
				, cyng::table::data_generator(ts, 0, status)
				, 1);
		}
	}

	void storage::merge_profile_meta_8181C78613FF(cyng::buffer_t srv_id
		, std::uint64_t days
		, std::chrono::system_clock::time_point ts
		, std::uint32_t status)
	{	//	24 h profile meta data
		auto const key = cyng::table::key_generator(srv_id, days + 1);
		auto const b = exists("TProfile_8181C78613FF", key);
		if (b) {
			//
			//	update
			//
			update("TProfile_8181C78613FF"
				, key
				, cyng::table::data_generator(ts, 0, status)
				, 2);
		}
		else {
			//
			//	insert
			//
			insert("TProfile_8181C78613FF"
				, key
				, cyng::table::data_generator(ts, 0, status)
				, 1);
		}
	}


	void storage::merge_profile_storage_8181C78611FF(cyng::buffer_t srv_id
		, std::uint64_t quarters
		, cyng::buffer_t code
		, cyng::object val
		, cyng::object scaler
		, cyng::object unit
		, cyng::object type)
	{	//	15 min profile readout data
		auto const key = cyng::table::key_generator(srv_id, quarters + 1, code);
		auto const b = exists("TStorage_8181C78611FF", key);

		auto const s = cyng::io::to_str(val);

		if (b) {
			//
			//	update
			//
			update("TStorage_8181C78611FF"
				, key
				, cyng::table::data_generator(s, type, scaler, unit)
				, 2);
		}
		else {
			//
			//	insert
			//
			insert("TStorage_8181C78611FF"
				, key
				, cyng::table::data_generator(s, type, scaler, unit)
				, 1);
		}

	}

	void storage::merge_profile_storage_8181C78612FF(cyng::buffer_t srv_id
		, std::uint64_t hours
		, cyng::buffer_t code
		, cyng::object val
		, cyng::object scaler
		, cyng::object unit
		, cyng::object type)
	{	//	60 min profile readout data
		auto const key = cyng::table::key_generator(srv_id, hours + 1, code);
		auto const b = exists("TStorage_8181C78612FF", key);

		auto const s = cyng::io::to_str(val);

		if (b) {
			//
			//	update
			//
			update("TStorage_8181C78612FF"
				, key
				, cyng::table::data_generator(s, type, scaler, unit)
				, 2);
		}
		else {
			//
			//	insert
			//
			insert("TStorage_8181C78612FF"
				, key
				, cyng::table::data_generator(s, type, scaler, unit)
				, 1);
		}
	}

	void storage::merge_profile_storage_8181C78613FF(cyng::buffer_t srv_id
		, std::uint64_t days
		, cyng::buffer_t code
		, cyng::object val
		, cyng::object scaler
		, cyng::object unit
		, cyng::object type)
	{	//	24 h profile readout data
		auto const key = cyng::table::key_generator(srv_id, days + 1, code);
		auto const b = exists("TStorage_8181C78613FF", key);

		auto const s = cyng::io::to_str(val);

		if (b) {
			//
			//	update
			//
			//
			//	update
			//
			update("TStorage_8181C78613FF"
				, key
				, cyng::table::data_generator(s, type, scaler, unit)
				, 2);
		}
		else {
			//
			//	insert
			//
			insert("TStorage_8181C78613FF"
				, key
				, cyng::table::data_generator(s, type, scaler, unit)
				, 1);
		}
	}

	std::uint64_t storage::shrink(std::uint16_t max_size, cyng::buffer_t srv_id, sml::obis profile)
	{
		switch (profile.to_uint64()) {
		case sml::CODE_PROFILE_1_MINUTE:
			return shrink(max_size, srv_id, "TProfile_8181C78610FF", "TStorage_8181C78610FF");
		case sml::CODE_PROFILE_15_MINUTE:
			return shrink(max_size, srv_id, "TProfile_8181C78611FF", "TStorage_8181C78611FF");
		case sml::CODE_PROFILE_60_MINUTE:
			return shrink(max_size, srv_id, "TProfile_8181C78612FF", "TStorage_8181C78612FF");
		case sml::CODE_PROFILE_24_HOUR:
			return shrink(max_size, srv_id, "TProfile_8181C78613FF", "TStorage_8181C78613FF");
		case sml::CODE_PROFILE_LAST_2_HOURS:
			break;
		case sml::CODE_PROFILE_LAST_WEEK:
			break;
		case sml::CODE_PROFILE_1_MONTH:
			break;
		case sml::CODE_PROFILE_1_YEAR:
			break;
		case sml::CODE_PROFILE_INITIAL:
			break;
		default:
			break;
		}
		return 0;
	}

	std::uint64_t storage::shrink(std::uint16_t max_size, cyng::buffer_t srv_id, std::string tbl_meta, std::string tbl_data)
	{
		//
		//	NOTE: this function produces sometimes an "bad parameter or other API misuse" error
		//

		auto s = get_session();

		//
		//	get the size of the table: SELECT COUNT(*) FROM ... WHERE 
		//
		auto cmd = create_cmd(tbl_meta, s.get_dialect());
		auto sql = cmd.select().count().where(cyng::sql::column(1) == cyng::sql::make_placeholder())();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			stmt->push(cyng::make_object(srv_id), 9);
			if (auto res = stmt->get_result()) {

				//
				//	size of the table 
				//
				auto const obj = res->get(1, cyng::TC_UINT64, 0);
				auto const count = cyng::numeric_cast<std::uint64_t>(obj, 0);
				stmt->close();	//	destroy prepared statement

				if (count > max_size) {

					//
					//	max size exceeded
					//	table have to be shrinked
					//	select hex(clientID), datetime(actTime) from TProfile_8181C78612FF order by actTime asc;
					//
					sql = cmd.select().pk().where(cyng::sql::column(1) == cyng::sql::make_placeholder()).order_by("actTime asc")();
					stmt = s.create_statement();
					r = stmt->prepare(sql);
					if (r.second) {

						stmt->push(cyng::make_object(srv_id), 9);
						auto counter = count - max_size;

						//
						//	collect keys to delete
						//
						cyng::table::key_list_t keys;
						while (auto res = stmt->get_result()) {

							BOOST_ASSERT(res->column_count() == 2);

							auto const id = res->get(1, cyng::TC_BUFFER, 9);
							auto const ts = res->get(2, cyng::TC_UINT64, 0);
							keys.push_back(cyng::table::key_generator(id, ts));

							if (counter == 0)	break;
							--counter;
						}

						stmt->close();	//	destroy prepared statement

						//
						//	delete collected keys
						//	in one transaction
						//
						sql = cmd.remove().by_key()();
						stmt = s.create_statement();
						r = stmt->prepare(sql);
						if (r.second) {

							BOOST_ASSERT(r.first == 2);
							s.begin();
							for (auto const& pk : keys) {
								stmt->push(pk.front(), 9);
								stmt->push(pk.back(), 0);
								if (stmt->execute()) {
									stmt->clear();
								}
								else {
									//	error
									break;
								}
							}
							s.commit();
						}

						auto cmd_data = std::move(create_cmd(tbl_data, s.get_dialect()));
						sql = cmd_data.remove().where(cyng::sql::column(1) == cyng::sql::placeholder() && cyng::sql::column(2) == cyng::sql::placeholder())();
						r = stmt->prepare(sql);
						if (r.second) {

							BOOST_ASSERT(r.first == 2);
							s.begin();
							for (auto const& pk : keys) {
								stmt->push(pk.front(), 9);
								stmt->push(pk.back(), 0);
								if (stmt->execute()) {
									stmt->clear();
								}
								else {
									//	error
									break;
								}
							}
							s.commit();
						}
					}

					return count - max_size;	//	rows deleted
				}
			}
		}
		return 0;
	}

	void storage::trx_start()
	{
		get_session().begin();
	}

	void storage::trx_commit()
	{
		get_session().commit();
	}

	void storage::trx_rollback()
	{
		get_session().rollback();
	}

	//
	//	initialize static member
	//
	cyng::table::meta_map_t const storage::mm_(storage::get_meta_map());

}
