/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "cluster.h"
#include "sync.h"
#include "setup_defs.h"
#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/table/meta.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, std::size_t tsk
		, cluster_config_t const& cfg)
	: base_(*btp)
	, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), btp->get_id()))
	, logger_(logger)
	, storage_tsk_(tsk)
	, config_(cfg)
	, master_(0)
	, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running");

		//
		//	ToDo: implement in data handler task
		//
		bus_->vm_.async_run(cyng::register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "db.trx.start");
		}));
		bus_->vm_.async_run(cyng::register_function("db.trx.commit", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, "db.trx.commit");
		}));

		bus_->vm_.async_run(cyng::register_function("db.insert", 6, std::bind(&cluster::db_insert, this, std::placeholders::_1)));

		//
		//	request handler
		//
		bus_->vm_.async_run(cyng::register_function("cluster.task.resume", 3, std::bind(&cluster::task_resume, this, std::placeholders::_1)));
		bus_->vm_.async_run(cyng::register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1)));

		//
		//	init cache
		//
		create_cache();
	}

	void cluster::run()
	{	
		CYNG_LOG_INFO(logger_, "connect to redundancy [ "
		<< master_
		<< " ] "
		<< config_[master_].host_
		<< ':'
		<< config_[master_].service_);

		bus_->vm_.async_run(bus_req_login(config_[master_].host_
			, config_[master_].service_
			, config_[master_].account_
			, config_[master_].pwd_
			, "setup"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent");

	}

	void cluster::stop()
	{
		bus_->vm_.async_run(bus_shutdown());
		CYNG_LOG_INFO(logger_, "cluster is stopped");
	}

	//
	//	slot [0] - login response from cluster
	//
	cyng::continuation cluster::process(cyng::version const& v)
	{
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_WARNING(logger_, "insufficient cluster protocol version: "	<< v);
		}

		auto r = cyng::async::start_task_delayed<sync>(base_.mux_
			, std::chrono::seconds(1)
			, logger_
			, bus_
			, storage_tsk_	//	storage task
			, "TDevice"
			, cache_);

		//
		//	start data synchronisation - load cache
		//
		if (r.second)
		{
			base_.mux_.send(storage_tsk_, 0, cyng::tuple_factory("TDevice", r.first));
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "could not start <sync> task TDevice");
		}

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

	void cluster::db_insert(cyng::context& ctx)
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
		//CYNG_LOG_TRACE(logger_, "db.insert - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");

		const auto state = cache_.get_state(table);
		CYNG_LOG_TRACE(logger_, "db.insert( " 
			<< table
			<< " ) - state: "
			<< state);

		if (state == TS_SYNC)
		{
			cache_.access([this, &frame](cyng::store::table* tbl)->void {

				cyng::table::key_type key;
				key = cyng::value_cast<cyng::table::key_type>(frame.at(1), key);

				//cyng::table::data_type data;
				//data = cyng::value_cast<cyng::table::data_type>(frame.at(2), data);

				//std::uint64_t gen = cyng::value_cast<std::uint64_t>(frame.at(3), 0);
				//std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(5), 0);

				if (tbl->exist(key))
				{
					CYNG_LOG_WARNING(logger_, "cache "
						<< tbl->meta().get_name()
						<< " clean up record "
						<< cyng::io::to_str(key));

					tbl->erase(key);
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

					base_.mux_.send(storage_tsk_, 1, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });
				}

			}, cyng::store::write_access(table));
		}
		else if (state == TS_READY)
		{
			//
			//	insert into SQL database
			//
			base_.mux_.send(storage_tsk_, 1, cyng::tuple_t{ frame.at(0), frame.at(1), frame.at(2), frame.at(3) });

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
		base_.mux_.send(tsk, slot, cyng::tuple_t(frame.begin() + 2, frame.end()));
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
		if (config_.size() > 1)
		{
			master_++;
			if (master_ == config_.size())
			{
				master_ = 0;
			}
			CYNG_LOG_INFO(logger_, "switch to redundancy "
				<< config_[master_].host_
				<< ':'
				<< config_[master_].service_);

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}

		//
		//	trigger reconnect 
		//
		CYNG_LOG_INFO(logger_, "reconnect to cluster in "
			<< config_[master_].monitor_.count()
			<< " seconds");
		base_.suspend(config_[master_].monitor_);

	}

	void cluster::create_cache()
	{
		CYNG_LOG_TRACE(logger_, "create cache tables");

		cache_.create_table(cyng::table::make_meta_table<1, 7>("TDevice",
			{ "pk", "name", "number", "descr", "id", "vFirmware", "enabled", "created" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
			{ 36, 128, 128, 512, 64, 64, 0, 0 }));

		cache_.create_table(cyng::table::make_meta_table<1, 13>("TGateway", { "pk"	//	primary key
			, "id"	//	(1) Server-ID (i.e. 0500153B02517E)
			, "manufacturer"	//	(2) manufacturer (i.e. EMH)
			, "model"	//	(3) Typbezeichnung (i.e. Variomuc ETHERNET)
			, "made"	//	(4) production date
			, "vFirmware"	//	(5) firmwareversion (i.e. 11600000)
			, "factoryNr"	//	(6) fabrik nummer (i.e. 06441734)
			, "ifService"	//	(7) MAC of service interface
			, "ifData"	//	(8) MAC od data interface
			, "pwdDef"	//	(9) Default PW
			, "pwdRoot"	//	(10)
			, "mbus"	//	(11)
			, "userName"	//	(12)
			, "userPwd"	//	(13)
			},
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_MAC48, cyng::TC_MAC48, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 23, 64, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 }));

	}


}
