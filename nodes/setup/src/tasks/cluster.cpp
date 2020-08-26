/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include "sync.h"
#include "tables.h"
#include "setup_defs.h"
#include <smf/shared/db_schemes.h>
#include <smf/cluster/generator.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/table/meta.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::store::db& cache
		, std::size_t tsk
		, cluster_config_t const& cfg)
	: base_(*btp)
	, bus_(bus_factory(btp->mux_, logger, cluster_tag, btp->get_id()))
	, logger_(logger)
	, storage_tsk_(tsk)
	, config_(cfg)
	, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	init cache
		//
		create_cache();

		//
		//	ToDo: implement in data handler task
		//
		bus_->vm_.register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, ctx.get_name());
			});
		bus_->vm_.register_function("db.trx.commit", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
			});

		bus_->vm_.register_function("bus.res.subscribe", 6, std::bind(&cluster::res_subscribe, this, std::placeholders::_1));
		bus_->vm_.register_function("db.req.insert", 4, std::bind(&cluster::db_req_insert, this, std::placeholders::_1));
		bus_->vm_.register_function("db.res.insert", 4, std::bind(&cluster::db_res_insert, this, std::placeholders::_1));
		bus_->vm_.register_function("db.req.modify.by.attr", 3, std::bind(&cluster::db_req_modify_by_attr, this, std::placeholders::_1));
		bus_->vm_.register_function("db.req.modify.by.param", 3, std::bind(&cluster::db_req_modify_by_param, this, std::placeholders::_1));
		//	ToDo: check if "db.req.remove" requires 3 instead of 2 parameters
		bus_->vm_.register_function("db.req.remove", 2, std::bind(&cluster::db_remove, this, std::placeholders::_1));

		//
		//	request handler
		//
		bus_->vm_.register_function("cluster.task.resume", 2, std::bind(&cluster::task_resume, this, std::placeholders::_1));
		bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1));
		bus_->vm_.register_function("bus.req.push.data", 0, std::bind(&cluster::bus_req_push_data, this, std::placeholders::_1));

		bus_->vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));
	}

	cyng::continuation cluster::run()
	{	
		//
		//	disconnect all sync tasks from cache
		//
		base_.mux_.post("node::sync", 2, cyng::tuple_t());

		BOOST_ASSERT_MSG(!bus_->vm_.is_halted(), "cluster bus is halted");
		bus_->vm_.async_run(bus_req_login(config_.get().host_
			, config_.get().service_
			, config_.get().account_
			, config_.get().pwd_
			, config_.get().auto_config_
			, config_.get().group_
			, "setup"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent to "
			<< config_.get().host_
			<< ':'
			<< config_.get().service_);

		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::stop(bool shutdown)
	{
        bus_->stop();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped - leaving cluster");
	}

	//
	//	slot [0] - login response from cluster
	//
	cyng::continuation cluster::process(cyng::version const& v)
	{
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_ERROR(logger_, "insufficient cluster protocol version: "	<< v);
		}

		//
		//	start syncing with table TDevice
		//
		sync_table("TDevice");

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
	{
		//
		//	switch to other configuration
		//
		reconfigure_impl();

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process(std::string table, std::size_t tsk)
	{
		CYNG_LOG_INFO(logger_, "task "
			<< tsk
			<< " synchronized table "
			<< table);

		//
		//	Tables are sync'd in the following order:
		//
		//	see tables::list_
		//	* TDevice
		//	* TGateway
		//	* TLoRaDevice
		//	* TMeter
		//	* TMeterwMBUS
		//	* TLL
		//	* TGUIUser
		//	* TNodeNames
		//	* TIECBridge
		//	* TGWSnapshot
		//	* TLoraUplink
		//

		//
		//	get next table to synchronize
		//

		auto pos = tables::find(table);

		while (pos != std::end(tables::list_)) {
			++pos;
			if (pos != std::end(tables::list_) && pos->cache_) {
				 
				CYNG_LOG_INFO(logger_, "sync table "
					<< pos->name_
					<< "["
					<< std::distance(std::begin(tables::list_), pos)
					<< "/"
					<< tables::list_.size()
					<< "]");

				sync_table(pos->name_);

				break;
			}
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::sync_table(std::string table)
	{
		auto r = cyng::async::start_task_delayed<sync>(base_.mux_
			, std::chrono::milliseconds(1)
			, logger_
			, bus_
			, base_.get_id()	//	cluster task
			, table
			, cache_);

		//
		//	start data synchronisation - load cache
		//
		if (r.second)
		{
			base_.mux_.post(storage_tsk_
				, 0	//	slot[0]
				, cyng::tuple_factory(table, r.first, bus_->vm_.tag()));
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "could not start <sync> task " << table);
		}
	}

	void cluster::res_subscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[TDevice,[763ae055-449c-4783-b383-8fc8cd52f44f],[2018-01-23 13:02:05.66872930,true,vFirmware,id,descr,number,name],72,aa7dc32f-91ff-4b08-89fc-53afb244a6a9,3]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	* origin session id
		//	* optional task id
		//	
		CYNG_LOG_TRACE(logger_, "res.subscribe - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");

		const auto state = cache_.get_state(table);
		CYNG_LOG_TRACE(logger_, "res.subscribe( " 
			<< table
			<< " ) - state: "
			<< state);

		if (state == TS_SYNC)
		{
			cache_.access([&](cyng::store::table* tbl)->void {

				cyng::table::key_type key;
				key = cyng::value_cast<cyng::table::key_type>(frame.at(1), key);

				if (tbl->exist(key))
				{
					CYNG_LOG_WARNING(logger_, "cache "
						<< tbl->meta().get_name()
						<< " clean up record "
						<< cyng::io::to_str(key));

					tbl->erase(key, ctx.tag());
				}
				else
				{
					//
					//	insert into SQL database
					//
					//CYNG_LOG_TRACE(logger_, "cache "
					//	<< tbl->meta().get_name()
					//	<< " insert record "
					//	<< cyng::io::to_str(key));

					base_.mux_.post(storage_tsk_, 1, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });
				}

			}, cyng::store::write_access(table));
		}
		else if (state == TS_READY)
		{
			//
			//	insert into SQL database
			//
			base_.mux_.post(storage_tsk_, 1, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });

		}
	}

	void cluster::db_req_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[TDevice,[763ae055-449c-4783-b383-8fc8cd52f44f],[2018-01-23 13:02:05.66872930,true,vFirmware,id,descr,number,name],72,aa7dc32f-91ff-4b08-89fc-53afb244a6a9,3]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		CYNG_LOG_TRACE(logger_, "db.req.insert - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
		const auto state = cache_.get_state(table);

		CYNG_LOG_DEBUG(logger_, "db.req.insert - table " 
			<< table
			<< " in state "
			<< ((state == TS_READY) ? "READY" : "SYNC"));

		//
		//	insert into SQL database
		//
		base_.mux_.post(storage_tsk_, 1, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });

	}

	void cluster::db_res_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[TDevice,[763ae055-449c-4783-b383-8fc8cd52f44f],[2018-01-23 13:02:05.66872930,true,vFirmware,id,descr,number,name],72,aa7dc32f-91ff-4b08-89fc-53afb244a6a9,3]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		//CYNG_LOG_TRACE(logger_, "db.res.insert - " << cyng::io::to_str(frame));

		//
		//	db.res.insert doesn't make sense for setup node
		//
		CYNG_LOG_TRACE(logger_, "db.res.insert - skipped");
		//const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
		//const auto state = cache_.get_state(table);
		//if (state == TS_READY)
		//{
		//	//
		//	//	insert into SQL database
		//	//
		//	base_.mux_.post(storage_tsk_, 1, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });
		//}
		//else
		//{
		//	CYNG_LOG_ERROR(logger_, "db.res.insert( "
		//		<< table
		//		<< " ) - wrong state: "
		//		<< state);
		//}
	}

	void cluster::db_req_modify_by_attr(cyng::context& ctx)
	{
		//	[TDevice,[eaec7649-80d5-4b71-8450-3ee2c7ef4917],(4:ipt:store)]
		//
		//	* table name
		//	* record key
		//	* attr [column,value]
		//	* gen
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.req.modify.by.attr - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");

		const auto state = cache_.get_state(table);

		if (state == TS_READY)
		{
			CYNG_LOG_TRACE(logger_, "db.req.modify.by.attr( "
				<< table
				<< " ) - state: "
				<< state);

			//	ToDo: implement slot for attributes
			//base_.mux_.post(storage_tsk_, 2, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2) });

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "db.req.modify.by.attr( "
				<< table
				<< " ) - wrong state: "
				<< state);

		}
	}

	void cluster::db_req_modify_by_param(cyng::context& ctx)
	{
		//	
		//
		//	* table name
		//	* record key
		//	* param [name,value]
		//	* gen
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "db.req.modify.by.param - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");

		const auto state = cache_.get_state(table);

		if (state == TS_READY)
		{
			CYNG_LOG_TRACE(logger_, "db.req.modify.by.param( "
				<< table
				<< " ) - state: "
				<< state);

			base_.mux_.post(storage_tsk_, 2, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "db.req.modify.by.param( "
				<< table
				<< " ) - wrong state: "
				<< state);

		}
	}

	void cluster::db_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		//
		//	[TDevice,[3723d50f-b745-4f0d-acdb-123706995493]]
		//
		//	* table name
		//	* record key
		//	
		CYNG_LOG_TRACE(logger_, "db.remove - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");
		const auto state = cache_.get_state(table);
		if (state == TS_READY)
		{
			//
			//	remove from SQL database
			//
			base_.mux_.post(storage_tsk_, 3, cyng::tuple_t{ frame.at(0), frame.at(1) });
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "db.remove( "
				<< table
				<< " ) - wrong state: "
				<< state);

		}
	}

	void cluster::task_resume(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[1,0,TDevice,3]
		CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
		std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
		std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);

		//cyng::tuple_t tpl(frame.begin() + 2, frame.end());
		base_.mux_.post(tsk, slot, cyng::tuple_t(frame.begin() + 2, frame.end()));
	}

	void cluster::bus_req_push_data(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[1,TLoraMeta,[5fff3e47-b434-4170-a5e3-9d2ed99f3262],[F03D291000001180,2015-10-22 13:39:59.48900000,0001,1,1,0,148,3233...3538,444eefd3,-99,7,12,G1,LC2,1,29000071,100000728,52,5],40326cbe-d41e-4285-8500-26e3253813af]
		//
		//	* seq (cluster)
		//	* channel/table
		//	* key
		//	* data
		//	* source
		//
		CYNG_LOG_TRACE(logger_, "bus.req.push.data - " << cyng::io::to_str(frame));

		//
		//	effectively this is an SQL insert
		//
		const std::string table = cyng::value_cast<std::string>(frame.at(1), "");
		const auto state = cache_.get_state(table);

		CYNG_LOG_DEBUG(logger_, "bus.req.push.data - table "
			<< table
			<< " in state "
			<< ((state == TS_READY) ? "READY" : "SYNC"));

		//
		//	insert into SQL database
		//
		base_.mux_.post(storage_tsk_, 1, cyng::tuple_t{ frame.at(1), frame.at(2), frame.at(3), cyng::make_object<std::size_t>(0u) });


	}

	void cluster::reconfigure(cyng::context& ctx)
	{
		reconfigure_impl();
	}

	void cluster::reconfigure_impl()
	{
		//
		//	switch to other master
		//
		if (config_.next())
		{
			CYNG_LOG_INFO(logger_, "switch to redundancy "
				<< config_.get().host_
				<< ':'
				<< config_.get().service_);
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}

		//
		//	trigger reconnect 
		//
		CYNG_LOG_INFO(logger_, "reconnect to cluster in "
			<< config_.get().monitor_.count()
			<< " seconds");
		base_.suspend(config_.get().monitor_);

	}

	void cluster::create_cache()
	{
		CYNG_LOG_TRACE(logger_, "create "
			<< tables::list_.size()
			<< " cache tables");

		for (auto const& tbl : tables::list_) {
			if (tbl.cache_) {
				if (!create_table(cache_, tbl.name_))
				{
					CYNG_LOG_FATAL(logger_, "cannot create table " << tbl.name_);
				}
			}
		}
	}
}
