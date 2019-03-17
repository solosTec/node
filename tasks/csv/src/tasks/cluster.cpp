/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"
#include "storage_db.h"
#include "profile_15_min.h"
#include "profile_60_min.h"
#include "profile_24_h.h"

#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, std::string language
		, cluster_config_t const& cfg_cluster
		, cyng::param_map_t const& cfg_db
		, cyng::param_map_t const& cfg_clock_day
		, cyng::param_map_t const& cfg_clock_hour
		, cyng::param_map_t const& cfg_clock_month
		, cyng::param_map_t const& cfg_trigger)
	: base_(*btp)
		, bus_(bus_factory(btp->mux_, logger, cluster_tag, btp->get_id()))
		, logger_(logger)
        , config_(cfg_cluster)
		, cfg_db_(cfg_db)
		, cfg_clock_day_(cfg_clock_day)
		, cfg_clock_hour_(cfg_clock_hour)
		, cfg_clock_month_(cfg_clock_month)
		, offset_(cyng::find_value(cfg_trigger, "offset", 7))
		, frame_(cyng::find_value(cfg_trigger, "frame", 7))
		, format_(cyng::find_value<std::string>(cfg_trigger, "format", "SML"))
		, language_(language)
		, profile_15_min_tsk_(cyng::async::NO_TASK)
		, profile_60_min_tsk_(cyng::async::NO_TASK)
		, profile_24_h_tsk_(cyng::async::NO_TASK)
		, storage_task_(cyng::async::NO_TASK)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

        //
        //	implement request handler
        //
        bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&cluster::reconfigure, this, std::placeholders::_1));
        bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));

	}

	cyng::continuation cluster::run()
	{	
        if (!bus_->is_online()) {
            connect();
        }
        else    {
            CYNG_LOG_DEBUG(logger_, "cluster bus is online");
        }

		return cyng::continuation::TASK_CONTINUE;

	}

	void cluster::stop()
	{
        //
        //	sign off from cloud
        //
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

		//
		//	insert instance into table _CSV
		//
		make_public();

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
	{
		//
		//	stop clocks and storage tasks
		//
		stop_sub_tasks();

        //
        //  switch to other configuration
        //
        reconfigure_impl();
		return cyng::continuation::TASK_CONTINUE;
	}

    void cluster::connect()
    {

        BOOST_ASSERT_MSG(!bus_->vm_.is_halted(), "cluster bus is halted");
        bus_->vm_.async_run(bus_req_login(config_.get().host_
            , config_.get().service_
            , config_.get().account_
            , config_.get().pwd_
            , config_.get().auto_config_
            , config_.get().group_
            , "csv"));

        CYNG_LOG_INFO(logger_, "cluster login request is sent to "
            << config_.get().host_
            << ':'
            << config_.get().service_);
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

	void cluster::start_sub_tasks()
	{
		CYNG_LOG_INFO(logger_, "connect to configuration database");

		storage_task_ = cyng::async::start_task_delayed<storage_db>(base_.mux_
			, std::chrono::seconds(2)
			, logger_
			, language_
			, bus_
			, cfg_db_
			, cfg_clock_day_
			, cfg_clock_hour_
			, cfg_clock_month_).first;

		CYNG_LOG_INFO(logger_, "start clocks");

        profile_15_min_tsk_ = cyng::async::start_task_delayed<profile_15_min>(base_.mux_
            , std::chrono::seconds(3)
            , logger_
            , storage_task_
            , offset_
			, frame_
			, format_).first;

		profile_60_min_tsk_ = cyng::async::start_task_delayed<profile_60_min>(base_.mux_
			, std::chrono::seconds(4)
			, logger_
			, storage_task_
			, offset_
			, frame_
			, format_).first;

		profile_24_h_tsk_ = cyng::async::start_task_delayed<profile_24_h>(base_.mux_
            , std::chrono::seconds(5)
			, logger_
			, storage_task_
			, offset_
			, frame_
			, format_).first;
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

		if (profile_60_min_tsk_ != cyng::async::NO_TASK) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop clock #"
				<< profile_60_min_tsk_);

			base_.mux_.stop(profile_60_min_tsk_);
			profile_60_min_tsk_ = cyng::async::NO_TASK;
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

	void cluster::make_public()
	{
		//
		//	create entry in table _CSV
		//
		bus_->vm_.async_run(bus_req_db_insert("_CSV"
			, cyng::table::key_generator(bus_->vm_.tag())
			, cyng::table::data_generator(format_
				, "SML"
				, offset_
				, frame_
				, std::chrono::system_clock::now()
				, std::chrono::system_clock::now()
				, std::chrono::system_clock::now()
				, 0u
				, 0u
				, 0u)
			, 1	//	generation
			, bus_->vm_.tag()));
	}

}
