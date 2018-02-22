/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "storage_db.h"
#include "write_db.h"
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
#include <cyng/vm/generator.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	storage_db::storage_db(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite")))
		, meta_map_(init_meta_map())
		, lines_()
		, rng_()
		, hit_list_()
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
		BOOST_ASSERT(pool_.get_pool_size() != 0);
		for (auto tsk : hit_list_)
		{
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop.writer " 
				<< tsk);
			base_.mux_.send(tsk, 1, cyng::tuple_t{});
		}
		hit_list_.clear();
		base_.suspend(std::chrono::seconds(1));
	}

	void storage_db::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< " stopped");
	}

	cyng::continuation storage_db::process(std::uint32_t channel, std::uint32_t source, std::string const& target, cyng::buffer_t const& data)
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
			//	create a new task
			auto tp = cyng::async::make_task<write_db>(base_.mux_
				, logger_
				, rng_()
				, channel
				, source
				, target
				, pool_.get_session()
				, meta_map_);

			//
			//	provide storage functionality
			//
			cyng::async::task_cast<write_db>(tp)->vm_.run(cyng::register_function("stop.writer", 1, std::bind(&storage_db::stop_writer, this, std::placeholders::_1)));
			cyng::async::task_cast<write_db>(tp)->init();

			//	start task
			auto res = cyng::async::start_task_sync(base_.mux_, tp);


			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> processing line "
				<< std::dec
				<< source
				<< ':'
				<< channel
				<< ':'
				<< target
				<< " ==> new task "
				<< res.first);

			//	insert into task list
			lines_[line] = res.first;

			//	take the new task
			base_.mux_.send(res.first, 0, cyng::tuple_factory(data));

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
				<< ':'
				<< target
				<< " ==> task "
				<< pos->second);

			//	take the existing task
			if (!base_.mux_.send(pos->second, 0, cyng::tuple_factory(data)))
			{
				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> processing line "
					<< std::dec
					<< source
					<< ':'
					<< channel
					<< " no task "
					<< pos->second);
			}
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

			return EXIT_SUCCESS;
		}
		std::cerr << "connect to database failed" << std::endl;
		return EXIT_FAILURE;
	}

	void storage_db::stop_writer(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		auto tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);

		for (auto pos = lines_.begin(); pos != lines_.end(); ++pos)
		{
			if (pos->second == tsk)
			{
				CYNG_LOG_INFO(logger_, "remove.writer " << tsk);
				lines_.erase(pos);
				hit_list_.push_back(tsk);
				return;
			}
		}
		CYNG_LOG_ERROR(logger_, "stop.writer " << tsk << " not found");
	}

	cyng::table::mt_table storage_db::init_meta_map()
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

		return meta_map;
	}
}
