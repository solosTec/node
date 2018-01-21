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
	cyng::continuation storage_db::process(std::string name, std::size_t tsk)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> merge & upload " 
			<< name
			<< " -> "
			<< tsk);

		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());
			cmd.select().all();
			std::string sql = cmd.to_str();
			CYNG_LOG_TRACE(logger_, sql);

			auto stmt = s.create_statement();
			stmt->prepare(sql);

			//
			//	lookup cache
			//
			cache_.access([this, meta, stmt](cyng::store::table* tbl)->void {

				while (auto res = stmt->get_result())
				{
					//
					//	convert SQL result to record
					//
					cyng::table::record rec = cyng::to_record(meta, res);

					//
					//	search cache for same record
					//
					cyng::table::record entry = tbl->lookup(rec.key());
					if (entry.empty())
					{
						//	cache miss -> upload
						CYNG_LOG_TRACE(logger_, "cache miss");
					}
					else
					{
						//	cache hit -> merge and delete
						CYNG_LOG_TRACE(logger_, "cache hit - delete record from " << meta->get_name());

						//
						//	ToDo: merge
						//

						//
						//	delete this record
						//
						tbl->erase(rec.key());
					}
				}
			}, cyng::store::write_access(name));

			//
			//	cache updated
			//
			base_.mux_.send(tsk, 0, cyng::tuple_factory(cache_.size()));
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

	int storage_db::init_db(cyng::tuple_t tpl)
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
			std::cout << r.first << std::endl;

			auto meta_map = storage_db::init_meta_map();
			for (auto tbl : meta_map)
			{
				cyng::sql::command cmd(tbl.second, s.get_dialect());
				cmd.create();
				std::string sql = cmd.to_str();
				std::cout << sql << std::endl;
				s.execute(sql);
			}


			/*
			cmd.insert();
			sql = cmd.to_str();
			auto stmt = s.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.first == 10);	//	3 parameters to bind
			BOOST_ASSERT(r.second);

			//	bind parameters
			stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
				.push(cyng::make_object("Douglas Adams"), 128)
				.push(cyng::make_object("42"), 128)
				.push(cyng::make_object("in search for the last question"), 512)
				.push(cyng::make_object("ID"), 64)
				.push(cyng::make_object("vFirmware"), 64)
				.push(cyng::make_object(true), 0)
				.push(cyng::make_object(std::chrono::system_clock::now()), 0)
				.push(cyng::make_object("config"), 1024)
				.push(cyng::make_object(27ull), 0)
				;
			stmt->execute();
			stmt->clear();
			*/


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
		meta_map.emplace("TDevice", cyng::table::make_meta_table<1, 9>("TDevice",
			{ "pk", "name", "number", "descr", "id", "vFirmware", "enabled", "created", "config", "gen" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_PARAM_MAP, cyng::TC_UINT64 },
			{ 36, 128, 128, 512, 64, 64, 0, 0, 1024, 0 }));

		meta_map.emplace("TUser", cyng::table::make_meta_table<1, 6>("TUser",
			{ "pk", "name", "team", "priv_read", "priv_write", "priv_delete", "gen" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_BOOL, cyng::TC_BOOL, cyng::TC_BOOL, cyng::TC_UINT64 },
			{ 36, 128, 0, 0, 0, 0, 0 }));

		meta_map.emplace("TGroup", cyng::table::make_meta_table<1, 4>("TGroup",
			{ "id", "descr", "admin", "shortname", "gen" },
			{ cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_STRING, cyng::TC_UINT64 },
			{ 0, 256, 0, 64, 0 }));

		//	vFirmware: (i.e. MUC-ETHERNET-1.318_11332000)
		//	factoryNr: (i.e. 06441734)
		//	mbus: W-Mbus ID (i.e. A815408943050131)
		meta_map.emplace("TGateway", cyng::table::make_meta_table<1, 14>("TGateway",
			{ "pk", "serverId", "manufacturer", "model", "proddata", "vFirmare", "factoryNr", "ifService", "ifData", "pwdDef", "pwdRoot", "mbus", "userName", "userPwd", "gen" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT64 },
			{ 36, 23, 64, 64, 0, 64, 8, 18, 18, 32, 32, 16, 32, 32, 0 }));

		//	id: (i.e. 1EMH0006441734)
		//	vParam: parameterization software version (i.e. 16A098828.pse)
		//	vFirmware: (i.e. 11600000)
		//	item: artikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
		//	class: Metrological Class: A, B, C, Q3/Q1, ...
		meta_map.emplace("TMeter", cyng::table::make_meta_table<1, 9>("TMeter",
			{ "pk", "id", "manufacturer", "proddata", "vFirmare", "vParam", "factoryNr", "item", "class", "gen" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT64 },
			{ 36, 64, 64, 0, 64, 64, 8, 128, 8, 0 }));

		return meta_map;
	}

}
