/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "storage_db.h"
#include "clock.h"
#include <NODE_project_info.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
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
		, cyng::param_map_t cfg_db
		, cyng::param_map_t cfg_trigger
		, cyng::param_map_t cfg_csv)
	: base_(*btp)
		, logger_(logger)
		, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg_db["type"], "SQLite")))
		, cfg_trigger_(cfg_trigger)
		, cfg_csv_(cfg_csv)
		, schema_(cyng::value_cast<std::string>(cfg_db["db-schema"], NODE_SUFFIX))
		, meta_map_(init_meta_map(schema_))
		, state_(TASK_STATE_INITIAL_)
		, clock_tsk_(cyng::async::NO_TASK)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		if (!pool_.start(cfg_db))
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

	cyng::continuation storage_db::run()
	{	
		switch (state_) {
		case TASK_STATE_INITIAL_:

			//
			//	check db session pool
			//
			if (pool_.get_pool_size() == 0) {

				CYNG_LOG_FATAL(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> empty connection pool");

				return cyng::continuation::TASK_STOP;
			}

			//
			//	update task state
			//
			state_ = TASK_STATE_OPEN_;

			//
			//	start clock
			//
			clock_tsk_ = cyng::async::start_task_delayed<clock>(base_.mux_
				, std::chrono::seconds(2)
				, logger_
				, base_.get_id()
				, std::chrono::minutes(cyng::find_value(cfg_trigger_, "offset", 7))).first;

			break;
		default:
			CYNG_LOG_INFO(logger_, "storage_db is running");
			break;
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_db::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");

	}

	//	slot 0 - merge
	cyng::continuation storage_db::process(std::chrono::system_clock::time_point start
		, std::chrono::system_clock::time_point end
		, std::chrono::minutes interval)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> generate CSV file from "
			<< cyng::to_str(start)
			<< " to "
			<< cyng::to_str(end)
			<< " stepsize "
			<< cyng::to_str(interval));

		//
		//	get all available meters in the specified time range
		//
		//	SELECT server FROM TSMLMeta WHERE ((actTime > julianday('2018-08-06 00:00:00')) AND (actTime < julianday('2018-08-07 00:00:00'))) GROUP BY server;

		auto pos = meta_map_.find("TSMLMeta");
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());
			cmd.select()[cyng::sql::column(8)].where(cyng::sql::column(5) > cyng::sql::make_constant(start) && cyng::sql::column(5) < cyng::sql::make_constant(end)).group_by(cyng::sql::column(8));
			std::string sql = cmd.to_str();
			CYNG_LOG_TRACE(logger_, sql);	//	select ... from name

			auto stmt = s.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			std::vector<std::string> server_ids;
			BOOST_ASSERT(r.second);
			if (r.second) {
				while (auto res = stmt->get_result()) {

					//
					//	convert SQL result to record
					//
					server_ids.push_back(cyng::value_cast<std::string>(res->get(1, cyng::TC_STRING, 23), "server"));
					CYNG_LOG_TRACE(logger_, server_ids.back());

				}
			}
			else {

			}

			stmt->close();

			//	select * from TSMLMeta INNER JOIN TSMLData ON TSMLMeta.pk = TSMLData.pk WHERE ((actTime > julianday('2018-08-06 00:00:00')) AND (actTime < julianday('2018-08-07 00:00:00')) AND server = '01-e61e-13090016-3c-07') ORDER BY trxID
			//	select * from TSMLMeta INNER JOIN TSMLData ON TSMLMeta.pk = TSMLData.pk WHERE ((actTime > julianday(?)) AND (actTime < julianday(?)) AND server = ?) ORDER BY trxID
			sql = "select TSMLMeta.pk, trxID, msgIdx, datetime(roTime), datetime(actTime), valTime, gateway, server, status, source, channel, target, OBIS, unitCode, unitName, dataType, scaler, val, result FROM TSMLMeta INNER JOIN TSMLData ON TSMLMeta.pk = TSMLData.pk WHERE ((actTime > julianday(?)) AND (actTime < julianday(?)) AND server = ?) ORDER BY actTime";
			r = stmt->prepare(sql);
			BOOST_ASSERT(r.second);
			for (auto id : server_ids) {
				if (r.second) {
					stmt->push(cyng::make_object(start), 0)
						.push(cyng::make_object(end), 0)
						.push(cyng::make_object(id), 23)
						;
					while (auto res = stmt->get_result()) {
						//	pk|trxID|msgIdx|roTime|actTime|valTime|gateway|server|status|source|channel|target|pk|OBIS|unitCode|unitName|dataType|scaler|val|result
						//	43a2a6bb-f45f-48e3-a2b7-74762f1752c1|41091175|1|2458330.95813657|2458330.97916667|118162556|00:15:3b:02:17:74|01-e61e-29436587-bf-03|0|0|0|Gas2|43a2a6bb-f45f-48e3-a2b7-74762f1752c1|0000616100ff|255|counter|u8|0|0|0

						auto trx = cyng::value_cast<std::string>(res->get(2, cyng::TC_STRING, 0), "TRX");
						auto ro_time = cyng::value_cast(res->get(4, cyng::TC_TIME_POINT, 0), std::chrono::system_clock::now());
						auto act_time = cyng::value_cast(res->get(5, cyng::TC_TIME_POINT, 0), std::chrono::system_clock::now());
						CYNG_LOG_TRACE(logger_, id
							<< ", "
							<< trx
							<< ", "
							//<< cyng::to_str(ro_time)
							//<< ", "
							<< cyng::to_str(act_time)
							<< ", "
							<< cyng::value_cast<std::string>(res->get(13, cyng::TC_STRING, 24), "OBIS")
							<< ", "
							<< cyng::value_cast<std::string>(res->get(19, cyng::TC_STRING, 512), "result")
						);
					}
				}
				stmt->clear();
			}
			stmt->close();
		}
		else
		{
			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown table TSMLMeta");
		}

		//	select * from TSMLMeta INNER JOIN TSMLData ON TSMLMeta.pk = TSMLData.pk;
		//	201
		//	pk|trxID|msgIdx|roTime|actTime|valTime|gateway|server|status|source|channel|target|pk|OBIS|unitCode|unitName|dataType|scaler|val|result
		//	43a2a6bb-f45f-48e3-a2b7-74762f1752c1|41091175|1|2458330.95813657|2458330.97916667|118162556|00:15:3b:02:17:74|01-e61e-29436587-bf-03|0|0|0|Gas2|43a2a6bb-f45f-48e3-a2b7-74762f1752c1|0000616100ff|255|counter|u8|0|0|0

		//	select * from TSMLMeta INNER JOIN TSMLData ON TSMLMeta.pk = TSMLData.pk where actTime > julianday('2018-08-01') AND actTime < julianday('2018-08-02') order by actTime;


		//auto pos = meta_map_.find(TSMLMeta);
		//if (pos != meta_map_.end())
		//{
		//	auto s = pool_.get_session();
		//	cyng::table::meta_table_ptr meta = (*pos).second;
		//	cyng::sql::command cmd(meta, s.get_dialect());
		//	cmd.select().all();
		//	std::string sql = cmd.to_str();
		//	CYNG_LOG_TRACE(logger_, sql);	//	select ... from name

		//	auto stmt = s.create_statement();
		//	stmt->prepare(sql);

		//	//
		//	//	lookup cache
		//	//
		//	//
		//	//	cache complete
		//	//	tell sync to synchronise the cache with master
		//	//
		//	base_.mux_.post(sync_tsk, 0, cyng::tuple_factory(name, 0));
		//}
		//else
		//{
		//	CYNG_LOG_FATAL(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> unknown table "
		//		<< name);
		//	base_.mux_.post(sync_tsk, 0, cyng::tuple_factory(name, 0));
		//}

		return cyng::continuation::TASK_CONTINUE;
	}


	cyng::table::mt_table storage_db::init_meta_map(std::string const& ver)
	{
		//
		//	SQL table schemes
		//
		std::map<std::string, cyng::table::meta_table_ptr> meta_map;

		if (boost::algorithm::equals(ver, "v4.0"))
		{
			meta_map.emplace("TSMLMeta", cyng::table::make_meta_table<1, 11>("TSMLMeta",
				{ "ident"
				, "trxID"	//	unique for every message
				, "midx"	//	message index
				, "roTime"	//	readout time
				, "actTime"	//	timestamp
				, "valTime"	//	seconds index
				, "gateway"
				, "server"
				, "meter"
				, "status"	//	M-Bus status
				, "source"
				, "channel" },
				{ cyng::TC_UUID		//	ident
				, cyng::TC_STRING	//	trxID
				, cyng::TC_UINT32	//	midx
				, cyng::TC_TIME_POINT	//	roTime
				, cyng::TC_TIME_POINT	//	actTime
				, cyng::TC_UINT32	//	valTime
				, cyng::TC_STRING	//	gateway
				, cyng::TC_STRING	//	server
				, cyng::TC_STRING	//	meter
				, cyng::TC_UINT32	//	status
				, cyng::TC_UINT32	//	source
				, cyng::TC_UINT32 },	//	channel
				{ 36	//	ident
				, 16	//	trxID
				, 0		//	midx
				, 0		//	roTime
				, 0		//	actTime
				, 0		//	valTime
				, 23	//	gateway
				, 23	//	server
				, 10	//	meter
				, 0		//	status
				, 0		//	source
				, 0 }));	//	channel

			meta_map.emplace("TSMLData", cyng::table::make_meta_table<2, 6>("TSMLData",
				{ "ident"
				, "OBIS"
				, "unitCode"
				, "unitName"
				, "dataType"
				, "scaler"
				, "val"		//	changed to val (since value is an SQL keyword)
				, "result" },
				{ cyng::TC_UUID		//	ident
				, cyng::TC_STRING	//	OBIS
				, cyng::TC_UINT8	//	unitCode
				, cyng::TC_STRING	//	unitName
				, cyng::TC_STRING	//	dataType
				, cyng::TC_INT32	//	scaler
				, cyng::TC_INT64	//	val
				, cyng::TC_STRING },	//	result
				{ 36	//	ident
				, 24	//	OBIS
				, 0		//	unitCode
				, 64	//	unitName
				, 16	//	dataType
				, 0		//	scaler
				, 0		//	val
				, 512 }));	//	result

		}
		else
		{

			//
			//	trxID - unique for every message
			//	msgIdx - message index
			//	status - M-Bus status
			//
			meta_map.emplace("TSMLMeta", cyng::table::make_meta_table<1, 11>("TSMLMeta",
				{ "pk", "trxID", "msgIdx", "roTime", "actTime", "valTime", "gateway", "server", "status", "source", "channel", "target" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_TIME_POINT, cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_STRING },
				{ 36, 16, 0, 0, 0, 0, 23, 23, 0, 0, 0, 32 }));

			//
			//	unitCode - physical unit
			//	unitName - descriptiv
			//	
			meta_map.emplace("TSMLData", cyng::table::make_meta_table<2, 6>("TSMLData",
				{ "pk", "OBIS", "unitCode", "unitName", "dataType", "scaler", "val", "result" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT8, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_INT32, cyng::TC_INT64, cyng::TC_STRING },
				{ 36, 24, 0, 64, 16, 0, 0, 512 }));
		}
		return meta_map;
	}


}
