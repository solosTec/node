/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_db.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/db/connection_types.h>
#include <cyng/db/session.h>
#include <cyng/db/interface_session.h>
#include <cyng/db/sql_table.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/sql.h>
#include <cyng/sql/dsl/binary_expr.hpp>
#include <cyng/sql/dsl/list_expr.hpp>
#include <cyng/sql/dsl/operators.hpp>
#include <cyng/sql/dsl/assign.hpp>
#include <cyng/sql/dsl/aggregate.h>
#include <cyng/table/meta.hpp>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	storage_db::storage_db(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite")))
		, meta_map_(init_meta_map())
		, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> established");

		if (!pool_.start(cfg))
		{
			CYNG_LOG_FATAL(logger_, "to start DB connection pool failed");
		}
		else
		{
			CYNG_LOG_INFO(logger_, "DB connection pool is running with "
				<< pool_.get_pool_size() 
				<< " connection(s)");
		}
	}

	void storage_db::run()
	{	
		CYNG_LOG_INFO(logger_, "storage_db is running");
	}

	void storage_db::stop()
	{
		CYNG_LOG_INFO(logger_, "storage_db is stopped");
	}

	//	slot 0 - merge
	cyng::continuation storage_db::process(std::string name, std::size_t sync_tsk, boost::uuids::uuid tag)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> load table "
			<< name);

		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());
			cmd.select().all();
			std::string sql = cmd.to_str();
			CYNG_LOG_TRACE(logger_, sql);	//	select ... from name

			auto stmt = s.create_statement();
			stmt->prepare(sql);

			//
			//	lookup cache
			//
			std::size_t counter{ 0 };
			cache_.access([&](cyng::store::table* tbl)->void {

				BOOST_ASSERT(meta->get_name() == tbl->meta().get_name());

				//
				//	read all results and populate cache
				//
				while (auto res = stmt->get_result())
				{
					//
					//	convert SQL result to record
					//
					cyng::table::record rec = cyng::to_record(meta, res);

					//
					//	ToDo: transform SQL record into cache record
					//
					if (!tbl->insert(rec.key(), rec.data(), rec.get_generation(), tag))
					{
						CYNG_LOG_ERROR(logger_, "copy record into cache " 
							<< meta->get_name()
							<< " failed - "
							<< cyng::io::to_str(rec.key()));
					}
					else
					{
						CYNG_LOG_TRACE(logger_, "insert record into cache "
							<< meta->get_name()
							<< " - "
							<< cyng::io::to_str(rec.key()));
						counter++;
					}
				}

				CYNG_LOG_INFO(logger_, "cache "
					<< meta->get_name()
					<< " is populated with "
					<< counter
					<< " records");

			}, cyng::store::write_access(name));

			//
			//	cache complete
			//	tell sync to synchronise the cache with master
			//
			BOOST_ASSERT(counter == cache_.size(name));
			base_.mux_.send(sync_tsk, 0, cyng::tuple_factory(name, counter));
		}
		else
		{
			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown table "
				<< name);
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation storage_db::process(std::string name
		, cyng::vector_t const& key
		, cyng::vector_t data
		, std::uint64_t gen)
	{
		std::reverse(data.begin(), data.end());

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> insert "
			<< name
			<< " "
			<< cyng::io::to_str(key)
			<< cyng::io::to_str(data)
			<< " - gen "
			<< gen);

		//
		//	insert into SQL database
		//
		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());
			cmd.insert();
			std::string sql = cmd.to_str();
			//CYNG_LOG_TRACE(logger_, sql);	//	insert

			auto stmt = s.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.second);

			if (boost::algorithm::equals(name, "TDevice"))
			{
				//	[763ae055-449c-4783-b383-8fc8cd52f44f]
				//	[2018-01-23 15:10:47.65306710,true,vFirmware,id,descr,number,name]
				//	bind parameters
				//	INSERT INTO TDevice (pk, gen, name, pwd, number, descr, id, vFirmware, enabled, creationTime, query) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
				BOOST_ASSERT(r.first == 11);	//	11 parameters to bind
				BOOST_ASSERT_MSG(key.size() == 1, "wrong TDevice key size");
				BOOST_ASSERT_MSG(data.size() == 9, "wrong TDevice data size");
				stmt->push(key.at(0), 36)	//	pk
					.push(cyng::make_object(gen), 0)	//	generation
					.push(data.at(0), 128)	//	name
					.push(data.at(1), 16)	//	password
					.push(data.at(2), 128)	//	
					.push(data.at(3), 512)
					.push(data.at(4), 64)
					.push(data.at(5), 64)	//	firmware
					.push(data.at(6), 0)	//	enabled
					.push(data.at(7), 0)	//	creationTime
					.push(data.at(8), 0)	//	query
					;
				if (!stmt->execute())
				{
					CYNG_LOG_ERROR(logger_, "sql insert failed: " << sql);
				}
				stmt->clear();

			}
			else
			{
				CYNG_LOG_ERROR(logger_, "unknown table " << name);	//	insert
			}
		}
		else
		{
			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown table "
				<< name);
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation storage_db::process(std::string name
		, cyng::vector_t const& key
		, cyng::param_t const& data
		, std::uint64_t gen)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> update "
			<< name
			<< ' '
			<< cyng::io::to_str(key)
			<< ' '
			<< data.first
			<< " = "
			<< cyng::io::to_str(data.second));

		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());

			//
			//	To add 1 hack is required since the column index starts with 1 (and not 0)
			//	The where clause works only for tables with a simple primary key.
			//
			std::pair<std::ptrdiff_t, bool> idx = meta->get_record_index(data.first);
			if (idx.second)
			{
				//
				//	update specific attribute
				//
				cmd.update(cyng::sql::make_assign(idx.first + 1, cyng::sql::make_placeholder())).where(cyng::sql::column(1) == cyng::sql::make_placeholder());

				std::string sql = cmd.to_str();
				//CYNG_LOG_TRACE(logger_, sql);	//	update

				auto stmt = s.create_statement();
				std::pair<int, bool> r = stmt->prepare(sql);
				if (!r.second)
				{
					//	maybe database connection was closed
					CYNG_LOG_FATAL(logger_, "sql prepare failed: " << sql);
					CYNG_LOG_INFO(logger_, "test if database connection was closed");
					//BOOST_ASSERT(r.second);
				}
				else
				{
					stmt->push(data.second, 0)
						.push(key.at(0), 36);
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql update failed: " << sql);
					}
					stmt->clear();
				}

				//
				//	update gen(eration)
				//
				cmd.update(cyng::sql::make_assign(2, cyng::sql::make_placeholder())).where(cyng::sql::column(1) == cyng::sql::make_placeholder());
				sql = cmd.to_str();
				//CYNG_LOG_TRACE(logger_, sql);	//	update

				//stmt = s.create_statement();
				r = stmt->prepare(sql);
				if (!r.second)
				{
					//	maybe database connection was closed
					CYNG_LOG_FATAL(logger_, "sql prepare failed: " << sql);
					CYNG_LOG_INFO(logger_, "test if datanase connection was closed");
					//BOOST_ASSERT(r.second);
				}
				else
				{
					stmt->push(cyng::make_object(gen), 0)
						.push(key.at(0), 36);
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql update failed: " << sql);
					}
					stmt->clear();
				}
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "sql update unknown index: " << data.first);

			}
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "unknown table "
				<< name);	//	insert
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation storage_db::process(std::string name
		, cyng::vector_t const& key)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> remove "
			<< name
			<< " "
			<< cyng::io::to_str(key));
		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());

			cmd.remove().by_key();	//	different key size
			std::string sql = cmd.to_str();
			CYNG_LOG_TRACE(logger_, sql);	//	update

			auto stmt = s.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.second);
			BOOST_ASSERT(r.first == key.size());

			//stmt->push(key.at(0), 36);
			for (int idx = 0; idx < key.size(); ++idx)
			{
				stmt->push(key.at(idx), 0);
			}
			if (!stmt->execute())
			{
				CYNG_LOG_ERROR(logger_, "sql delete failed: " << sql);
			}
			stmt->clear();

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "unknown table "
				<< name);	//	remove
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	int storage_db::init_db(cyng::tuple_t tpl, std::size_t count)
	{
		auto cfg = cyng::to_param_map(tpl);
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second)
		{
			//
			//	connect string
			//
			std::cout << "connect string: " << r.first << std::endl;

			auto meta_map = storage_db::init_meta_map();
			for (auto tbl : meta_map)
			{
				cyng::sql::command cmd(tbl.second, s.get_dialect());
				cmd.create();
				std::string sql = cmd.to_str();
				std::cout << sql << std::endl;
				s.execute(sql);

				if (tbl.first == "TDevice")
				{
					for (std::size_t idx = 0; idx < count; ++idx)
					{
						cmd.insert();
						sql = cmd.to_str();
						auto stmt = s.create_statement();
						std::pair<int, bool> r = stmt->prepare(sql);
						BOOST_ASSERT(r.first == 11);	//	11 parameters to bind
						BOOST_ASSERT(r.second);

						//	bind parameters
						stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
							.push(cyng::make_object<std::uint64_t>(idx), 0)
							.push(cyng::make_object("device-" + std::to_string(idx + 1)), 128)
							.push(cyng::make_object("secret"), 16)
							.push(cyng::make_object(std::to_string(idx + 1001)), 128)
							.push(cyng::make_object("comment #" + std::to_string(idx + 1)), 512)
							.push(cyng::make_object("ID"), 64)
							.push(cyng::make_object("v" + std::to_string(idx + 1)), 64)
							.push(cyng::make_object(true), 0)
							.push(cyng::make_object(std::chrono::system_clock::now()), 0)
							.push(cyng::make_object<std::uint32_t>((((idx + 1)% 7) == 0) ? 319 : 6), 0)
							;
						stmt->execute();
						stmt->clear();
					}
				}
#ifdef _DEBUG
				else if (tbl.first == "TLL")
				{
					cmd.insert();
					sql = cmd.to_str();
					auto stmt = s.create_statement();
					std::pair<int, bool> r = stmt->prepare(sql);
					//BOOST_ASSERT(r.first == 9);	//	3 parameters to bind
					BOOST_ASSERT(r.second);

					//	bind parameters
					stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(14ull), 0)
						.push(cyng::make_object("Leased Line -  1"), 128)
						.push(cyng::make_object(true), 0)
						.push(cyng::make_object(std::chrono::system_clock::now()), 0)
						;
					stmt->execute();
					stmt->clear();

					r = stmt->prepare(sql);
					//	bind parameters
					stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(441ull), 0)
						.push(cyng::make_object("Leased Line - 2"), 128)
						.push(cyng::make_object(false), 0)
						.push(cyng::make_object(std::chrono::system_clock::now()), 0)
						;
					stmt->execute();
					stmt->clear();
				}
#endif
			}



			return EXIT_SUCCESS;
		}
		std::cerr << "connect to database failed" << std::endl;
		return EXIT_FAILURE;
	}

	std::map<std::string, cyng::table::meta_table_ptr> storage_db::init_meta_map()
	{
		//
		//	SQL table scheme
		//
		std::map<std::string, cyng::table::meta_table_ptr> meta_map;
		meta_map.emplace("TDevice", cyng::table::make_meta_table<1, 10>("TDevice",
			{ "pk", "gen", "name", "pwd", "msisdn", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },
			{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_UINT32 },
			{ 36, 0, 128, 16, 128, 512, 64, 64, 0, 0, 0 }));

		meta_map.emplace("TUser", cyng::table::make_meta_table<1, 6>("TUser",
			{ "pk", "gen", "name", "team", "priv_read", "priv_write", "priv_delete" },
			{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_BOOL, cyng::TC_BOOL, cyng::TC_BOOL },
			{ 36, 0, 128, 0, 0, 0, 0 }));

		meta_map.emplace("TGroup", cyng::table::make_meta_table<1, 4>("TGroup",
			{ "id", "gen", "descr", "admin", "shortname" },
			{ cyng::TC_UINT32, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_STRING },
			{ 0, 0, 256, 0, 64 }));

		//	vFirmware: (i.e. MUC-ETHERNET-1.318_11332000)
		//	factoryNr: (i.e. 06441734)
		//	mbus: W-Mbus ID (i.e. A815408943050131)
		meta_map.emplace("TGateway", cyng::table::make_meta_table<1, 14>("TGateway",
			{ "pk", "gen", "serverId", "manufacturer", "model", "proddata", "vFirmare", "factoryNr", "ifService", "ifData", "pwdDef", "pwdRoot", "mbus", "userName", "userPwd" },
			{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 0, 23, 64, 64, 0, 64, 8, 18, 18, 32, 32, 16, 32, 32 }));

		//	id: (i.e. 1EMH0006441734)
		//	vParam: parameterization software version (i.e. 16A098828.pse)
		//	vFirmware: (i.e. 11600000)
		//	item: artikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
		//	class: Metrological Class: A, B, C, Q3/Q1, ...
		meta_map.emplace("TMeter", cyng::table::make_meta_table<1, 9>("TMeter",
			{ "pk", "gen", "id", "manufacturer", "proddata", "vFirmare", "vParam", "factoryNr", "item", "class" },
			{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 0, 64, 64, 0, 64, 64, 8, 128, 8 }));

		meta_map.emplace("TLL", cyng::table::make_meta_table<2, 4>("TLL",
			{ "first"
			, "second"
			, "gen"
			, "descr"
			, "enabled"
			, "creationTime" },
			{ cyng::TC_UUID, cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
			{ 36, 36, 0, 128, 0, 0 }));

		meta_map.emplace("TSysMsg", cyng::table::make_meta_table<1, 3>("TSysMsg",
			{ "id"
			, "ts"
			, "severity"
			, "msg" },
			{ cyng::TC_UINT64, cyng::TC_TIME_POINT, cyng::TC_UINT8, cyng::TC_STRING },
			{ 0, 0, 0, 256 }));

		return meta_map;
	}

}
