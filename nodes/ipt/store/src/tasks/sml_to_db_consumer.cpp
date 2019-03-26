/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "sml_to_db_consumer.h"
#include "../message_ids.h"
#include "../../../../../nodes/shared/db/db_meta.h"

#include <smf/sml/defs.h>
#include <NODE_project_info.h>

#include <cyng/async/task/base_task.h>
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
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	sml_db_consumer::sml_db_consumer(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t ntid	//	network task id
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, ntid_(ntid)
		, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite")))
		, schema_(cyng::value_cast<std::string>(cfg["db-schema"], NODE_SUFFIX))
		, period_(cyng::value_cast(cfg["period"], 12))
		, meta_map_(init_meta_map(schema_))
		, task_state_(TASK_STATE_INITIAL)
		, lines_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		if (!boost::algorithm::equals(schema_, NODE_SUFFIX))
		{
			CYNG_LOG_WARNING(logger_, "use non-standard db schema " << schema_);
		}

		if (!boost::algorithm::equals(schema_, NODE_SUFFIX))
		{
			CYNG_LOG_WARNING(logger_, "use non-standard db schema " << schema_);
		}

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

	cyng::continuation sml_db_consumer::run()
	{
		BOOST_ASSERT(pool_.get_pool_size() != 0);
		switch (task_state_) {
		case TASK_STATE_INITIAL:

			//
			//	test connection pool
			//
			if (pool_.get_pool_size() == 0) {
				CYNG_LOG_FATAL(logger_, "DB connection pool is empty");
				return cyng::continuation::TASK_STOP;
			}
			task_state_ = TASK_STATE_DB_OK;
			break;

		case TASK_STATE_DB_OK:

			//
			//	register as SML:XML consumer 
			//
			register_consumer();
			task_state_ = TASK_STATE_REGISTERED;
			break;

		default:
			//CYNG_LOG_TRACE(logger_, base_.get_class_name()
			//	<< " processed "
			//	<< msg_counter_
			//	<< " messages");
			break;
		}

		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void sml_db_consumer::stop(bool shutdown)
	{
		//
		//	remove all open lines
		//
		lines_.clear();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation sml_db_consumer::process(std::uint64_t line
		, std::string target)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " create line "
			<< line
			<< ':'
			<< target);

		lines_.emplace(std::piecewise_construct,
			std::forward_as_tuple(line),
			std::forward_as_tuple(meta_map_
				, schema_
				, (std::uint32_t)((line & 0xFFFFFFFF00000000LL) >> 32)
				, (std::uint32_t)(line & 0xFFFFFFFFLL)
				, target));

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> has "
			<< lines_.size()
			<< " active lines");

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation sml_db_consumer::process(std::uint64_t line
		, std::uint16_t code
		, std::size_t idx
		, cyng::tuple_t msg)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " line "
			<< line
			<< " received #"
			<< idx
			<< ':'
			<< sml::messages::name(code));

		auto pos = lines_.find(line);
		if (pos != lines_.end()) {

			try {
				//
				//	write to DB
				//
				pos->second.write(pool_.get_session(), msg, idx);
			}
			catch (std::exception const& ex) {

				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< " line "
					<< line
					<< " error: "
					<< ex.what());
			}
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " line "
				<< line
				<< " not found");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [2] - CONSUMER_REMOVE_LINE
	cyng::continuation sml_db_consumer::process(std::uint64_t line)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " close line "
			<< line);

		auto pos = lines_.find(line);
		if (pos != lines_.end()) {

			//
			//	remove this line
			//
			lines_.erase(pos);
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< " line "
				<< line
				<< " not found");
		}

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> has "
			<< lines_.size()
			<< " active lines");

		return cyng::continuation::TASK_CONTINUE;
	}

	//	EOM
	cyng::continuation sml_db_consumer::process(std::uint64_t line, std::size_t idx, std::uint16_t crc)
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	int sml_db_consumer::init_db(cyng::tuple_t tpl)
	{
		auto cfg = cyng::to_param_map(tpl);
		auto schema = cyng::value_cast<std::string>(cfg["db-schema"], NODE_SUFFIX);
		if (!boost::algorithm::equals(schema, NODE_SUFFIX))
		{
			std::cerr << "WARNING: use compatibility schema: " << schema << std::endl;
		}

		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second)
		{
			//
			//	connect string
			//
			std::cout << r.first << std::endl;

			auto meta_map = sml_db_consumer::init_meta_map(schema);
			for (auto tbl : meta_map)
			{
				cyng::sql::command cmd(tbl.second, s.get_dialect());
				cmd.create();
				std::string sql = cmd.to_str();
				std::cout << sql << std::endl;
				s.execute(sql);
			}

			return EXIT_SUCCESS;
		}
		std::cerr << "connect to database failed" << std::endl;
		return EXIT_FAILURE;
	}

	void sml_db_consumer::register_consumer()
	{
		base_.mux_.post(ntid_, STORE_EVENT_REGISTER_CONSUMER, cyng::tuple_factory("SML:DB", base_.get_id()));
	}

	cyng::table::mt_table sml_db_consumer::init_meta_map(std::string const& ver)
	{
		//
		//	SQL table schemes
		//
		std::map<std::string, cyng::table::meta_table_ptr> meta_map;

		if(boost::algorithm::equals(ver, "v4.0"))
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
			meta_map.emplace("TSMLMeta", create_meta("TSMLMeta"));

			//
			//	unitCode - physical unit
			//	unitName - descriptiv
			//	
			meta_map.emplace("TSMLData", create_meta("TSMLData"));
		}
		return meta_map;
	}

	//
	//	version 1
	//	CREATE TABLE TSMLMeta (ident TEXT, trxID TEXT, midx INT, roTime FLOAT, actTime FLOAT, valTime INT, gateway TEXT, server TEXT, meter TEXT, status INT, source INT, channel INT, PRIMARY KEY(ident));
	//	CREATE TABLE TSMLData (ident TEXT, OBIS TEXT, unitCode INT, unitName TEXT, dataType TEXT, scaler INT, val INT, result TEXT, PRIMARY KEY(ident, OBIS));
	//

}
