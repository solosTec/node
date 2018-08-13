/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include "storage_db.h"
#include "profile_15_min.h"
#include "profile_24_h.h"

#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cluster_config_t const& cfg_cluster
		, cyng::param_map_t const& cfg_db
		, cyng::param_map_t const& cfg_clock_day
		, cyng::param_map_t const& cfg_clock_month
		, cyng::param_map_t const& cfg_trigger)
	: base_(*btp)
		, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), btp->get_id()))
		, logger_(logger)
		, cfg_cluster_(cfg_cluster)
		, cfg_db_(cfg_db)
		, cfg_clock_day_(cfg_clock_day)
		, cfg_clock_month_(cfg_clock_month)
		, cfg_trigger_(cfg_trigger)
		, profile_15_min_tsk_(cyng::async::NO_TASK)
		, profile_24_h_tsk_(cyng::async::NO_TASK)
		, storage_task_(cyng::async::NO_TASK)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation cluster::run()
	{	
		bus_->vm_.async_run(bus_req_login(cfg_cluster_.get().host_
			, cfg_cluster_.get().service_
			, cfg_cluster_.get().account_
			, cfg_cluster_.get().pwd_
			, cfg_cluster_.get().auto_config_
			, cfg_cluster_.get().group_
			, "csv"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent to "
			<< cfg_cluster_.get().host_
			<< ':'
			<< cfg_cluster_.get().service_);

		return cyng::continuation::TASK_CONTINUE;

	}

	void cluster::stop()
	{
        bus_->stop();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped - leaving cluster");
	}

	//	slot 0
	cyng::continuation cluster::process(cyng::version const& v)
	{
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_WARNING(logger_, "insufficient cluster protocol version: "	<< v);
		}

		//
		//	start clocks and storage tasks
		//
		start_sub_tasks();

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
	{
		//
		//	stop clocks and storage tasks
		//
		stop_sub_tasks();

		//
		//	switch to other master
		//
		if (cfg_cluster_.next())
		{ 
			CYNG_LOG_INFO(logger_, "switch to redundancy " 
				<< cfg_cluster_.get().host_
				<< ':'
				<< cfg_cluster_.get().service_);

			bus_->vm_.async_run(bus_req_login(cfg_cluster_.get().host_
				, cfg_cluster_.get().service_
				, cfg_cluster_.get().account_
				, cfg_cluster_.get().pwd_
				, cfg_cluster_.get().auto_config_
				, cfg_cluster_.get().group_
				, "task:csv"));

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::start_sub_tasks()
	{
		CYNG_LOG_INFO(logger_, "connect to configuration database");

		storage_task_ = cyng::async::start_task_delayed<storage_db>(base_.mux_
			, std::chrono::seconds(2)
			, logger_
			, bus_
			, cfg_db_
			, cfg_clock_day_
			, cfg_clock_month_).first;

		CYNG_LOG_INFO(logger_, "start clocks");

        profile_15_min_tsk_ = cyng::async::start_task_delayed<profile_15_min>(base_.mux_
            , std::chrono::seconds(3)
            , logger_
            , storage_task_
            , cfg_trigger_).first;

		profile_24_h_tsk_ = cyng::async::start_task_delayed<profile_24_h>(base_.mux_
            , std::chrono::seconds(4)
			, logger_
			, storage_task_
			, cfg_trigger_).first;
	}

	void cluster::stop_sub_tasks()
	{
		CYNG_LOG_INFO(logger_, "stop the clocks");

		if (profile_15_min_tsk_ != cyng::async::NO_TASK) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop clock #"
				<< profile_15_min_tsk_);

			base_.mux_.stop(profile_15_min_tsk_);
			profile_15_min_tsk_ = cyng::async::NO_TASK;
		}

		if (profile_24_h_tsk_ != cyng::async::NO_TASK) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop clock #"
				<< profile_24_h_tsk_);

			base_.mux_.stop(profile_24_h_tsk_);
			profile_24_h_tsk_ = cyng::async::NO_TASK;
		}

		if (storage_task_ != cyng::async::NO_TASK) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop storage task #"
				<< storage_task_);
			base_.mux_.stop(storage_task_);
			storage_task_ = cyng::async::NO_TASK;
		}
	}
}
