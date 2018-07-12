/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "storage_db.h"
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
	storage_db::storage_db(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite")))
		, schema_(cyng::value_cast<std::string>(cfg["db-schema"], NODE_SUFFIX))
		, meta_map_(init_meta_map(schema_))
		, lines_()
		, rng_()
		, hit_list_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> established");

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

	cyng::continuation storage_db::run()
	{
		BOOST_ASSERT(pool_.get_pool_size() != 0);
		if (!hit_list_.empty())
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop "
				<< hit_list_.size()
				<< " processor(s)");
			for (auto line : hit_list_)
			{
				//
				//	call destructor
				//
				lines_.erase(line);
			}
			hit_list_.clear();
		}
		else
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< lines_.size()
				<< " processor(s) online");

		}
		base_.suspend(std::chrono::seconds(30));

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_db::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_db::process(std::uint32_t channel
		, std::uint32_t source
		, std::string const& target
		, std::string const& protocol
		, cyng::buffer_t const& data)
	{
		//
		//	create the line ID by combining source and channel into one 64 bit integer
		//
		const std::uint64_t line = (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);

		//
		//	select/create a SML processor task
		//
		const auto pos = lines_.find(line);
		if (pos == lines_.end())
		{
			//
			//	create new processor
			//
			auto res = lines_.emplace(std::piecewise_construct,
				std::forward_as_tuple(line),
				std::forward_as_tuple(base_.mux_.get_io_service()
					, logger_
					, rng_()
					, channel
					, source
					, target
					, pool_.get_session()
					, schema_
					, meta_map_));

			if (res.second)
			{
				res.first->second.vm_.register_function("stop.writer", 1, std::bind(&storage_db::stop_writer, this, std::placeholders::_1));
				res.first->second.parse(data);	//	sync
				//res.first->second.vm_.async_run(cyng::generate_invoke("sml.parse", data));
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "startup processor for line "
					<< channel
					<< ':'
					<< source
					<< ':'
					<< target
					<< " failed");
			}
		}
		else
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> processing line "
				<< std::dec
				<< source
				<< ':'
				<< channel
				<< " with processor "
				<< pos->second.vm_.tag());

			pos->second.parse(data);
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	int storage_db::init_db(cyng::tuple_t tpl)
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

			auto meta_map = storage_db::init_meta_map(schema);
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

	void storage_db::stop_writer(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

		for (auto pos = lines_.begin(); pos != lines_.end(); ++pos)
		{
			if (pos->second.vm_.tag() == tag)
			{
				CYNG_LOG_INFO(logger_, "remove db processor " << tag);

				//
				//	stop VM (async)
				//
				pos->second.stop();

				//
				//	add to hitlist and remove later
				//
				hit_list_.push_back(pos->first);
				return;
			}
		}
		CYNG_LOG_ERROR(logger_, "db processor " << tag << " not found");
	}

	cyng::table::mt_table storage_db::init_meta_map(std::string const& ver)
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

	//
	//	version 1
	//	CREATE TABLE TSMLMeta (ident TEXT, trxID TEXT, midx INT, roTime FLOAT, actTime FLOAT, valTime INT, gateway TEXT, server TEXT, meter TEXT, status INT, source INT, channel INT, PRIMARY KEY(ident));
	//	CREATE TABLE TSMLData (ident TEXT, OBIS TEXT, unitCode INT, unitName TEXT, dataType TEXT, scaler INT, val INT, result TEXT, PRIMARY KEY(ident, OBIS));
	//

}
