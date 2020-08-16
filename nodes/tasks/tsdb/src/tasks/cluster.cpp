/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "cluster.h"

#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/dom/algorithm.h>
#include <cyng/tuple_cast.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cluster_config_t const& cfg_cluster
		, std::set<std::size_t> tasks)
	: base_(*btp)
		, bus_(bus_factory(btp->mux_, logger, cluster_tag, btp->get_id()))
		, logger_(logger)
        , config_(cfg_cluster)
		, dispatcher_(btp->mux_, logger, tasks)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	data handling
		//
		bus_->vm_.register_function("db.trx.start", 0, [this](cyng::context& ctx) {
			CYNG_LOG_TRACE(logger_, ctx.get_name());
			});
		bus_->vm_.register_function("db.trx.commit", 1, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
			});
		dispatcher_.register_this(bus_->vm_);

        //
        //	implement request handler
        //
		bus_->vm_.register_function("bus.res.subscribe", 6, std::bind(&cluster::res_subscribe, this, std::placeholders::_1));
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

	void cluster::stop(bool shutdown)
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
		//	insert instance into table _TimeSeriesParams
		//
		make_public();

		//
		//	sync tables
		//
		sync_table("_TimeSeries");

		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::sync_table(std::string const& name)
	{
		CYNG_LOG_INFO(logger_, "sync table " << name);

		//
		//	Get existing records from master. This could be setup data
		//	from another redundancy or data collected during a line disruption.
		//
		bus_->vm_.async_run(bus_req_subscribe(name, base_.get_id()));

	}

	cyng::continuation cluster::process()
	{
		//
		//	stop clocks and storage tasks
		//
		//dispatcher_.stop_sub_tasks();

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
            , "tsdb"));

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


	void cluster::res_subscribe(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	examples:
		//	[TDevice,[911fc4a1-8d9b-4d18-97f7-84a1cd576139],[00000006,2018-02-04 15:31:37.00000000,true,v88,ID,comment #88,1088,secret,device-88],88,dfa6b9a1-4170-41bd-8945-80b936059231,1]
		//	[TGateway,[dca135f3-ff2b-4bf7-8371-a9904c74522b],[operator,operator,mbus,pwd,user,00:01:02:03:04:06,00:01:02:03:04:05,factory-nr,VSES-1.13_1133000038X025d,2018-06-05 16:01:06.29959300,EMH-VSES,EMH,05000000000000],0,e197fc51-0f13-4643-968d-8d0332bae068,1]
		//	[*SysMsg,[14],[cluster member dash:63efc328-218a-4635-a582-1cb4ddc7af25 closed,4,2018-06-05 16:17:50.88472100],1,e197fc51-0f13-4643-968d-8d0332bae068,1]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	* origin session id
		//	* optional task id
		//	
		//CYNG_LOG_TRACE(logger_, "res.subscribe - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid,		//	[4] origin session id
			std::size_t				//	[5] optional task id
		>(frame);

		CYNG_LOG_TRACE(logger_, "res.subscribe "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));

		//
		//	reorder vectors
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		dispatcher_.res_subscribe(std::get<0>(tpl)	//	[0] table name
			, std::get<1>(tpl)	//	[1] table key
			, std::get<2>(tpl)	//	[2] record
			, std::get<3>(tpl)	//	[3] generation
			, std::get<4>(tpl)	//	[4] origin session id
			, std::get<5>(tpl));
	}

	void cluster::make_public()
	{
		//
		//	create entry in table _TimeSeriesParams
		//
		//bus_->vm_.async_run(bus_req_db_insert("_TimeSeriesParams"
		//	, cyng::table::key_generator(bus_->vm_.tag())
		//	, cyng::table::data_generator(format_
		//		, "SML"
		//		, offset_
		//		, frame_
		//		, std::chrono::system_clock::now()
		//		, std::chrono::system_clock::now()
		//		, std::chrono::system_clock::now()
		//		, 0u
		//		, 0u
		//		, 0u)
		//	, 1	//	generation
		//	, bus_->vm_.tag()));
	}

}
