/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "storage.h"
#include "segw.h"
#include <smf/sml/obis_db.h>
#include <smf/ipt/config.h>
#include <smf/ipt/scramble_key_format.h>
#include <smf/sml/parser/obis_parser.h>

#include <cyng/table/meta.hpp>
#include <cyng/db/interface_statement.h>
#include <cyng/db/sql_table.h>
#include <cyng/intrinsics/traits.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/vector_cast.hpp>
#include <cyng/parser/mac_parser.h>
#include <cyng/io/serializer.h>

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

		auto tmp = cmd.insert();
		boost::ignore_unused(tmp);

		std::string const sql = cmd.to_str();
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

		auto tmp = cmd.select().all();
		boost::ignore_unused(tmp);

		std::string const sql = cmd.to_str();
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
		}

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
			stmt->push(cyng::make_object(gen), 0);
			meta->loop_body([&](cyng::table::column&& col) {
				stmt->push(body.at(idx), col.width_);
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

		auto tmp = cmd.insert();
		boost::ignore_unused(tmp);

		std::string const sql = cmd.to_str();
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
			if (stmt->execute())	return true;
		}

		return false;
	}

	bool storage::remove(std::string tbl
		, cyng::table::key_type const& key)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		auto tmp = cmd.remove().by_key();
		boost::ignore_unused(tmp);

		std::string const sql = cmd.to_str();
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
		}

		return false;
	}

	std::uint64_t storage::count(std::string tbl)
	{
		auto s = pool_.get_session();
		auto cmd = create_cmd(tbl, s.get_dialect());
		if (!cmd.is_valid())	return false;

		auto tmp = cmd.select().count();
		boost::ignore_unused(tmp);

		std::string const sql = cmd.to_str();
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

		auto tmp = cmd.select().count().by_key();
		boost::ignore_unused(tmp);

		std::string const sql = cmd.to_str();
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
		auto const key = cyng::table::key_generator(srv_id, minutes);
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
		auto const key = cyng::table::key_generator(srv_id, quarters);
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
	{
		auto const key = cyng::table::key_generator(srv_id, hours);
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

	void storage::merge_profile_storage_8181C78611FF(cyng::buffer_t srv_id
		, std::uint64_t hours
		, cyng::buffer_t code
		, cyng::object val
		, cyng::object scaler
		, cyng::object unit
		, cyng::object type)
	{
		auto const key = cyng::table::key_generator(srv_id, hours, code);
		auto const b = exists("TStorage_8181C78611FF", key);

		auto const s = cyng::io::to_str(val);

		if (b) {
			//
			//	update
			//
		}
		else {
			//
			//	insert
			//
			insert("TStorage_8181C78611FF"
				, key
				, cyng::table::data_generator(s, scaler, unit, type)
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
	{
		auto const key = cyng::table::key_generator(srv_id, hours, code);
		auto const b = exists("TStorage_8181C78612FF", key);

		auto const s = cyng::io::to_str(val);

		if (b) {
			//
			//	update
			//
		}
		else {
			//
			//	insert
			//
			insert("TStorage_8181C78612FF"
				, key
				, cyng::table::data_generator(s, scaler, unit, type)
				, 1);
		}

	}

	//
	//	initialize static member
	//
	cyng::table::meta_map_t const storage::mm_(storage::get_meta_map());

	cyng::table::meta_vec_t create_storage_meta_data()
	{
		//
		//	SQL table scheme
		//
		return 
		{
			//
			//	Configuration table
			//
			cyng::table::make_meta_table_gen<1, 3>("TCfg",
			{ "path"	//	OBIS path, ':' separated values
			, "val"		//	value
			, "def"		//	default value
			, "type"	//	data type code (default)
			},
			{ cyng::TC_STRING
			, cyng::TC_STRING
			, cyng::TC_STRING
			, cyng::TC_UINT32
			},
			{ 128
			, 256
			, 256
			, 0
			}),

			//
			//	operation log (81 81 C7 89 E1 FF)
			//
			cyng::table::make_meta_table_gen<1, 11>("TOpLog",
				{ "ROWID"		//	index - with SQLite this prevents creating a column
								//	-- body
				, "actTime"		//	actual time
				, "regPeriod"	//	register period
				, "valTime"		//	val time
				, "status"		//	status word
				, "event"		//	event code
				, "peer"		//	peer address
				, "utc"			//	UTC time
				, "serverId"	//	server ID (meter)
				, "target"		//	target name
				, "pushNr"		//	operation number
				, "details"		//	description (DATA_PUSH_DETAILS)
				},
				{ cyng::TC_UINT64		//	index 
										//	-- body
				, cyng::TC_TIME_POINT	//	actTime
				, cyng::TC_UINT32		//	regPeriod
				, cyng::TC_TIME_POINT	//	valTime
				, cyng::TC_UINT64		//	status
				, cyng::TC_UINT32		//	event
				, cyng::TC_BUFFER		//	peer_address
				, cyng::TC_TIME_POINT	//	UTC time
				, cyng::TC_BUFFER		//	serverId
				, cyng::TC_STRING		//	target
				, cyng::TC_UINT8		//	push_nr
				, cyng::TC_STRING		//	details
				},
				{ 0		//	index
						//	-- body
				, 0		//	actTime
				, 0		//	regPeriod
				, 0		//	valTime
				, 0		//	status
				, 0		//	event
				, 13	//	peer_address
				, 0		//	utc
				, 23	//	serverId
				, 64	//	target
				, 0		//	push_nr
				, 128	//	string
				}),

			//
			//	Communication Profile
			//

			//
			//	Access Rights (User)
			//

			//
			//	Profile Configurations
			//
			create_profile_meta("TProfile_8181C78610FF"),	//	PROFILE_1_MINUTE
			create_profile_meta("TProfile_8181C78611FF"),	//	PROFILE_15_MINUTE
			create_profile_meta("TProfile_8181C78612FF"),	//	PROFILE_60_MINUTE
			create_profile_meta("TProfile_8181C78613FF"),	//	PROFILE_24_HOUR
			//create_meta_load_profile("TProfile_8181C78614FF"),	//	PROFILE_LAST_2_HOURS
			//create_meta_load_profile("TProfile_8181C78615FF"),	//	PROFILE_LAST_WEEK
			//create_meta_load_profile("TProfile_8181C78616FF"),	//	PROFILE_1_MONTH
			//create_meta_load_profile("TProfile_8181C78617FF"),	//	PROFILE_1_YEAR
			//create_meta_load_profile("TProfile_8181C78618FF"),	//	PROFILE_INITIAL

			//
			//	Readout data (one for each profile)
			//
			create_profile_storage("TStorage_8181C78610FF"),	//	PROFILE_1_MINUTE
			create_profile_storage("TStorage_8181C78611FF"),	//	PROFILE_15_MINUTE
			create_profile_storage("TStorage_8181C78612FF"),	//	PROFILE_60_MINUTE
			create_profile_storage("TStorage_8181C78613FF"),	//	PROFILE_24_HOUR
			//create_meta_data_storage("TStorage_8181C78614FF"),	//	PROFILE_LAST_2_HOURS
			//create_meta_data_storage("TStorage_8181C78615FF"),	//	PROFILE_LAST_WEEK
			//create_meta_data_storage("TStorage_8181C78616FF"),	//	PROFILE_1_MONTH
			//create_meta_data_storage("TStorage_8181C78617FF"),	//	PROFILE_1_YEAR
			//create_meta_data_storage("TStorage_8181C78618FF"),	//	PROFILE_INITIAL

			//
			//	Configuration/Status
			//	visible/active Devices
			//
			//
			//	That an entry of a mbus devices exists means 
			//	the device is visible
			//
			cyng::table::make_meta_table_gen<1, 11>("TDeviceMBUS",
				{ "serverID"	//	server/meter ID
				, "lastSeen"	//	last seen - Letzter Datensatz: 20.06.2018 14:34:22"
				, "class"		//	device class (always "---" == 2D 2D 2D)
				, "active"
				, "descr"
				//	---
				, "status"	//	"Statusinformation: 00"
				, "mask"	//	"Bitmaske: 00 00"
				, "interval"	//	"Zeit zwischen zwei Datensätzen: 49000"
								//	--- optional data
				, "pubKey"	//	Public Key: 18 01 16 05 E6 1E 0D 02 BF 0C FA 35 7D 9E 77 03"
				, "aes"		//	AES-Key
				, "user"
				, "pwd"
				},
				{ cyng::TC_BUFFER		//	server ID
				, cyng::TC_TIME_POINT	//	last seen
				, cyng::TC_STRING		//	device class
				, cyng::TC_BOOL			//	active
				, cyng::TC_STRING		//	manufacturer/description

				, cyng::TC_UINT32		//	status (81 00 60 05 00 00)
				, cyng::TC_UINT16		//	bit mask (81 81 C7 86 01 FF)
				, cyng::TC_UINT32		//	interval (milliseconds)
				, cyng::TC_BUFFER		//	pubKey
				, cyng::TC_AES128		//	AES 128 (16 bytes)
				, cyng::TC_STRING		//	user
				, cyng::TC_STRING		//	pwd
				},
				{ 9		//	serverID
				, 0		//	lastSeen
				, 16	//	device class
				, 0		//	active
				, 128	//	manufacturer/description

				, 0		//	status
				, 0		//	mask
				, 0		//	interval
				, 16	//	pubKey
				, 32	//	aes
				, 32	//	user
				, 32	//	pwd
				}),

			cyng::table::make_meta_table_gen<2, 4>("TDataCollector",
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	position/number - starts with 1
								//	-- body
				, "profile"		//	[OBIS] type 1min, 15min, 1h, ... (OBIS_PROFILE)
				, "active"		//	[bool] turned on/off (OBIS_DATA_COLLECTOR_ACTIVE)
				, "maxSize"		//	[u32] max entry count (OBIS_DATA_COLLECTOR_SIZE)
				, "regPeriod"	//	[seconds] register period - if 0, recording is event-driven (OBIS_DATA_REGISTER_PERIOD)
				},
				{ cyng::TC_BUFFER		//	serverID
				, cyng::TC_UINT8		//	nr
										//	-- body
				, cyng::TC_BUFFER		//	profile
				, cyng::TC_BOOL			//	active
				, cyng::TC_UINT16		//	maxSize
				, cyng::TC_SECOND		//	regPeriod
				},
				{ 9		//	serverID
				, 0		//	nr
						//	-- body
				, 6		//	profile
				, 0		//	active
				, 0		//	maxSize
				, 0		//	regPeriod
				}),

			//
			//	Push operations
			//	81 81 C7 8A 01 FF - OBIS_PUSH_OPERATIONS
			//
			cyng::table::make_meta_table_gen<2, 5>("TPushOps",
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	position/number - starts with 1
								//	-- body
				, "interval"	//	[u32] (81 18 C7 8A 02 FF) push interval in seconds
				, "delay"		//	[u32] (81 18 C7 8A 03 FF) push delay in seconds 
				, "source"		//	[OBIS] (81 81 C7 8A 04 FF) push source
				, "target"		//	[string] (81 47 17 07 00 FF) target name
				, "service"		//	[OBIS] (81 49 00 00 10 FF) push service
				},
				{ cyng::TC_BUFFER		//	serverID
				, cyng::TC_UINT8		//	nr
										//	-- body
				, cyng::TC_UINT32		//	interval
				, cyng::TC_UINT32		//	delay
				, cyng::TC_BUFFER		//	source
				, cyng::TC_STRING		//	target
				, cyng::TC_BUFFER		//	service
				},
				{ 9		//	serverID
				, 0		//	nr
						//	-- body
				, 0		//	interval
				, 0		//	delay
				, 6		//	source
				, 32	//	target
				, 6		//	service
				}),

			//
			//	data mirror - list of OBIS codes
			//	81 81 C7 8A 23 FF - DATA_COLLECTOR_OBIS
			//
			cyng::table::make_meta_table_gen<3, 2>("TDataMirror",
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	reference to TDataCollector.nr
				, "reg"			//	index
								//	-- body
				, "code"		//	OBIS code
				, "active"		//	[bool] turned on/off
				},
				{ cyng::TC_BUFFER		//	serverID
				, cyng::TC_UINT8		//	nr
				, cyng::TC_UINT8		//	reg
										//	-- body
				, cyng::TC_BUFFER		//	code
				, cyng::TC_BOOL			//	active
				},
				{ 9		//	serverID
				, 0		//	nr
				, 0		//	reg
						//	-- body
				, 6		//	code
				, 0		//	active
				})


		};
	}

	cyng::table::meta_table_ptr create_profile_meta(std::string name)
	{
		return cyng::table::make_meta_table_gen<2, 3>(name,
			{ "clientID"	//	server/meter/sensor ID
			, "tsidx"		//	time stamp index
							//	-- body
			, "actTime"		//
			, "valTime"		//	signed integer
			, "status"		//	status
			},
			{ cyng::TC_BUFFER		//	clientID
			, cyng::TC_UINT64		//	tsidx
									//	-- body
			, cyng::TC_TIME_POINT	//	actTime
			, cyng::TC_UINT32		//	valTime
			, cyng::TC_UINT32		//	status
			},
			{ 9		//	serverID
			, 0		//	tsidx
					//	-- body
			, 0		//	actTime
			, 0		//	valTime
			, 0		//	status
			});
	}

	cyng::table::meta_table_ptr create_profile_storage(std::string name)
	{
		return cyng::table::make_meta_table_gen<3, 4>(name,
			{ "clientID"	//	server/meter/sensor ID
			, "tsidx"		//	time stamp index
			, "OBIS"		//	server/meter ID
			, "val"			//	readout value
			, "type"		//	cyng data type
			, "scaler"		//	decimal place
			, "unit"		//	physical unit
			},
			{ cyng::TC_BUFFER	//	clientID
			, cyng::TC_UINT64	//	tsidx
			, cyng::TC_BUFFER	//	OBIS
								//	-- body
			, cyng::TC_STRING	//	val
			, cyng::TC_INT32	//	type code
			, cyng::TC_INT8		//	scaler
			, cyng::TC_UINT8	//	unit
			},
			{ 9		//	serverID
			, 0		//	tsidx
			, 6		//	OBIS
					//	-- body
			, 128	//	val
			, 0		//	type
			, 0		//	scaler
			, 0		//	unit
			});
	}

	bool init_storage(cyng::param_map_t&& cfg)
	{
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {

			//
			//	connect string
			//
#ifdef _DEBUG
			std::cout
				<< "connect string: ["
				<< r.first
				<< "]"
				<< std::endl
				;
#endif

			//
			//	start transaction
			//
			s.begin();


			for (auto const& tbl : storage::mm_)
			{
#ifdef _DEBUG
				std::cout
					<< "create table  : ["
					<< tbl.first
					<< "]"
					<< std::endl
					;
#endif
				cyng::sql::command cmd(tbl.second, s.get_dialect());
				auto sql = cmd.create()();
#ifdef _DEBUG
				std::cout
					<< sql
					<< std::endl;
#endif
				s.execute(sql);
			}

			//
			//	commit
			//
			s.commit();

			return true;
		}
		else {
			std::cerr
				<< "connect ["
				<< r.first
				<< "] failed"
				<< std::endl
				;
		}
		return false;
	}

	bool transfer_config_to_storage(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom)
	{
		//
		//	create a database session
		//
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {

			//
			//	start transaction
			//
			s.begin();

			//
			//	transfer IP-T configuration
			//
			{
				cyng::vector_t vec;
				vec = cyng::value_cast(dom.get("ipt"), vec);
				auto const cfg_ipt = ipt::load_cluster_cfg(vec);
				std::uint8_t idx{ 1 };
				for (auto const rec : cfg_ipt.config_) {
					//	host
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx)
						}), cyng::make_object(rec.host_));

					//	target port
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx)
						}), cyng::make_object(rec.service_));

					//	source port (unused)
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx)
						}), cyng::make_object(static_cast<std::uint16_t>(0u)));

					//	account
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx)
						}), cyng::make_object(rec.account_));

					//	password
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx)
						}), cyng::make_object(rec.pwd_));

					//	scrambled
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
						}, "scrambled"), cyng::make_object(rec.scrambled_));

					//	scramble key
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
						}, "sk"), cyng::make_object(ipt::to_string(rec.sk_)));

					// update index
					++idx;
				}

				//	ip-t reconnect time in minutes
				auto reconnect = cyng::numeric_cast<std::uint8_t>(dom["ipt-param"].get(sml::OBIS_TCP_WAIT_TO_RECONNECT.to_str()), 1u);
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM,
					sml::OBIS_TCP_WAIT_TO_RECONNECT
					}), cyng::make_object(reconnect));

				auto retries = cyng::numeric_cast<std::uint32_t>(dom["ipt-param"].get(sml::OBIS_TCP_CONNECT_RETRIES.to_str()), 3u);
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM,
					sml::OBIS_TCP_CONNECT_RETRIES
					}), cyng::make_object(retries));

				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM,
					OBIS_CODE(00, 80, 80, 00, 03, 01)
					}), cyng::make_object(0u));

				//
				//	master index (0..1)
				//
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM}, "master"), cyng::make_object(static_cast<std::uint8_t>(0u)));

			}

			//
			//	transfer IEC configuration
			//
			{
				//	get a tuple/list of params
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(dom.get("if-1107"), tpl);
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107,
							code
							}), param.second);
					}
					else {
						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer M-Bus configuration
			//
			{
				init_config_record(s, build_cfg_key({
					sml::OBIS_CLASS_MBUS
					}, "descr"), cyng::make_object("M-Bus Configuration"));

				//	get a tuple/list of params
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(dom.get("mbus"), tpl);
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						init_config_record(s, build_cfg_key({
							sml::OBIS_CLASS_MBUS,
							code
							}), param.second);
					}
					else {
						init_config_record(s, build_cfg_key({
							sml::OBIS_CLASS_MBUS
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer wireless-LMN configuration
			//	81 06 19 07 01 FF
			//
			{
				init_config_record(s, build_cfg_key({
					sml::OBIS_W_MBUS_PROTOCOL
					}, "descr"), cyng::make_object("wireless-LMN configuration (M-Bus)"));

				//	get a tuple/list of params
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(dom.get("wireless-LMN"), tpl);
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						init_config_record(s, build_cfg_key({
							sml::OBIS_W_MBUS_PROTOCOL,
							code
							}), param.second);
					}
					else {
						init_config_record(s, build_cfg_key({
							sml::OBIS_W_MBUS_PROTOCOL
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer wired-LMN configuration
			//	as part of IEC 62506-21 configuration
			//	81 81 C7 93 00 FF
			//
			{
				init_config_record(s, build_cfg_key({
					sml::OBIS_IF_1107
					}, "descr"), cyng::make_object("wired-LMN configuration (IEC 62506-21)"));

				//	get a tuple/list of params
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(dom.get("wired-LMN"), tpl);
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107,
							code
							}), param.second);
					}
					else if (boost::algorithm::equals(param.first, "databits")) {
						//	u8
						auto const val = cyng::numeric_cast<std::uint8_t>(param.second, 8u);
						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107
							}, param.first), cyng::make_object(val));
					}
					else {
						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer hardware configuration
			//
			{
				//	get a tuple/list of params
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(dom.get("hardware"), tpl);
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					if (boost::algorithm::equals(param.first, "mac")) {

						//	"mac": "00:ff:90:98:57:56"
						//
						//	05 + MAC = server ID
						//
						std::string rnd_mac_str;
						{
							using cyng::io::operator<<;
							std::stringstream ss;
							ss << cyng::generate_random_mac48();
							ss >> rnd_mac_str;
						}
						auto const mac = cyng::value_cast<std::string>(param.second, rnd_mac_str);

						std::pair<cyng::mac48, bool > const r = cyng::parse_mac48(mac);
						init_config_record(s, build_cfg_key({
							sml::OBIS_SERVER_ID
						}), cyng::make_object(r.second ? r.first : cyng::generate_random_mac48()));

					}
					else if (boost::algorithm::equals(param.first, "class")) {

						//	"class": "129-129:199.130.83*255", (81 81 C7 82 53 FF)
						auto const id = cyng::value_cast<std::string>(param.second, "129-129:199.130.83*255");
						auto const r = sml::parse_obis(id);
						if (r.second) {
							init_config_record(s, build_cfg_key({
								sml::OBIS_DEVICE_CLASS
							}), cyng::make_object(r.first.to_str()));
						}
						else {
							init_config_record(s, build_cfg_key({
								sml::OBIS_DEVICE_CLASS
								}), cyng::make_object(sml::OBIS_DEV_CLASS_MUC_LAN.to_str()));
						}
					}
					else if (boost::algorithm::equals(param.first, "manufacturer")) {

						//	"manufacturer": "ACME Inc.",
						auto const id = cyng::value_cast<std::string>(param.second, "solosTec");
						init_config_record(s, build_cfg_key({
							sml::OBIS_DATA_MANUFACTURER
							}), cyng::make_object(id));

					}
					else if (boost::algorithm::equals(param.first, "serial")) {

						//	"serial": 34287691,
						std::uint32_t const serial = cyng::numeric_cast<std::uint32_t>(param.second, 10000000u);
						init_config_record(s, build_cfg_key({
							sml::OBIS_SERIAL_NR
							}), cyng::make_object(serial));

					}

				}
			}

			//
			//	SML login: accepting all/wrong server IDs
			//
			init_config_record(s, "accept-all-ids", dom.get("accept-all-ids"));

			//
			//	OBIS logging cycle in minutes
			//
			{
				auto const val = cyng::value_cast(dom.get(sml::OBIS_OBISLOG_INTERVAL.to_str()), 15);
				init_config_record(s, sml::OBIS_OBISLOG_INTERVAL.to_str(), cyng::make_minutes(val));
			}

			//
			//	shifting storage time in seconds (affects all meters)
			//
			{
				auto const val = cyng::numeric_cast<std::int32_t>(dom.get(sml::OBIS_STORAGE_TIME_SHIFT.to_str()), 0);
				init_config_record(s, sml::OBIS_STORAGE_TIME_SHIFT.to_str(), cyng::make_object(val));
			}

			{

				//
				//	map all available GPIO paths
				//
				init_config_record(s, "gpio-path", dom.get("gpio-path"));

				auto const gpio_list = cyng::vector_cast<int>(dom.get("gpio-list"), 0);
				if (gpio_list.size() != 4) {
					std::cerr 
						<< "***warning: invalid count of gpios: " 
						<< gpio_list.size()
						<< std::endl;
				}
				std::stringstream ss;
				bool initialized{ false };
				for (auto const gpio : gpio_list) {
					if (initialized) {
						ss << ' ';
					}
					else {
						initialized = true;
					}
					ss
						<< gpio
						;
				}
				init_config_record(s, "gpio-vector", cyng::make_object(ss.str()));

				//
				//	commit
				//
				s.commit();
			}
			return true;
		}

		return false;
	}

	bool init_config_record(cyng::db::session& s, std::string const& key, cyng::object obj)
	{
		//
		//	ToDo: use already prepared statements
		//
		cyng::table::meta_table_ptr meta = storage::mm_.at("TCfg");
		cyng::sql::command cmd(meta, s.get_dialect());
		auto const sql = cmd.insert()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {
			//	repackaging as string
			auto const val = cyng::make_object(cyng::io::to_str(obj));
			stmt->push(cyng::make_object(key), 128);	//	pk
			stmt->push(cyng::make_object(1u), 0);	//	gen
			stmt->push(val, 256);	//	val
			stmt->push(val, 256);	//	def
			stmt->push(cyng::make_object(obj.get_class().tag()), 0);	//	type
			if (stmt->execute())	return true;
		}
		return false;
	}
}
