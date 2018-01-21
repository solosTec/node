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
	namespace ipt
	{

		storage_db::storage_db(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, cyng::param_map_t cfg)
		: base_(*btp)
			, logger_(logger)
			, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite")))
			, meta_map_(init_meta_map())
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

			//
			//	trxID - unique for every message
			//	msgIdx - message index
			//	status - M-Bus status
			//
			std::map<std::string, cyng::table::meta_table_ptr> meta_map;
			meta_map.emplace("TSMLMeta", cyng::table::make_meta_table<1, 10>("TSMLMeta",
				{ "pk", "trxID", "msgIdx", "roTime", "actTime", "valTime", "gateway", "server", "status", "source", "channel" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_TIME_POINT, cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32 },
				{ 36, 16, 0, 0, 0, 0, 23, 23, 0, 0, 0 }));

			//
			//	unitCode - physical unit
			//	unitName - descriptiv
			//	
			meta_map.emplace("TSMLData", cyng::table::make_meta_table<2, 6>("TSMLData",
				{ "pk", "OBIS", "unitCode", "unitName", "dataType", "scaler", "val", "result" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT8, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_INT32, cyng::TC_INT64, cyng::TC_STRING },
				{ 36, 24, 0, 64, 16, 0, 0, 512 }));

			return meta_map;
		}
	}
}
