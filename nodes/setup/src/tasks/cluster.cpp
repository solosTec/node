/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "cluster.h"
#include "sync.h"
#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
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

		bus_->vm_.async_run(cyng::register_function("db.insert", 5, std::bind(&cluster::db_insert, this, std::placeholders::_1)));

		//
		//	request handler
		//
		bus_->vm_.async_run(cyng::register_function("cluster.task.resume", 4, std::bind(&cluster::task_resume, this, std::placeholders::_1)));
		bus_->vm_.async_run(cyng::register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1)));

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

	//	slot 0
	cyng::continuation cluster::process(cyng::version const& v)
	{
		//std::cout << "simple::slot-0($" << base_.get_id() << ", v" << v.major() << "." << v.minor() << ")" << std::endl;
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_WARNING(logger_, "insufficient cluster protocol version: "	<< v);
		}

		//
		//	start data synchronisation
		//
		cyng::async::start_task_delayed<sync>(base_.mux_
			, std::chrono::seconds(1)
			, logger_
			, bus_
			, storage_tsk_
			, "TDevice"
			, cache_);

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
		//	[TDevice,[763ae055-449c-4783-b383-8fc8cd52f44f],[2018-01-14 17:13:10.94181940,true,vFirmware,id,descr,number,name],72,a3ed60cd-d4f1-4307-939a-f4cc2bbaf18f]
		CYNG_LOG_TRACE(logger_, "db.insert - " << cyng::io::to_str(frame));
		const std::string table = cyng::value_cast<std::string>(frame.at(0), "");

		cyng::table::key_type key;
		key = cyng::value_cast<cyng::table::key_type>(frame.at(1), key);

		cyng::table::data_type data;
		data = cyng::value_cast<cyng::table::data_type>(frame.at(2), data);

		std::uint64_t gen = cyng::value_cast<std::uint64_t>(frame.at(3), 0);

		if (!cache_.insert(table, key, data, gen))
		{
			CYNG_LOG_ERROR(logger_, "db.insert failed with "
				<< cyng::io::to_str(frame));
		}
	}

	void cluster::task_resume(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//	[1,0,TDevice,3]
		CYNG_LOG_TRACE(logger_, "resume task - " << cyng::io::to_str(frame));
		std::size_t tsk = cyng::value_cast<std::size_t>(frame.at(0), 0);
		std::size_t slot = cyng::value_cast<std::size_t>(frame.at(1), 0);
		base_.mux_.send(tsk, slot, cyng::tuple_t{ frame.at(2), frame.at(3) });
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

}
