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
#include <boost/uuid/random_generator.hpp>

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cluster_config_t const& cfg)
	: base_(*btp)
	, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), btp->get_id()))
	, logger_(logger)
	, config_(cfg)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	ToDo: implement request handler
		//

	}

	cyng::continuation cluster::run()
	{	
		bus_->vm_.async_run(bus_req_login(config_.get().host_
			, config_.get().service_
			, config_.get().account_
			, config_.get().pwd_
			, config_.get().auto_config_
			, config_.get().group_
			, "mqtt"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent to "
			<< config_.get().host_
			<< ':'
			<< config_.get().service_);

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
		//std::cout << "simple::slot-0($" << base_.get_id() << ", v" << v.major() << "." << v.minor() << ")" << std::endl;
		if (v < cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
		{
			CYNG_LOG_WARNING(logger_, "insufficient cluster protocol version: "	<< v);
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
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

			bus_->vm_.async_run(bus_req_login(config_.get().host_
				, config_.get().service_
				, config_.get().account_
				, config_.get().pwd_
				, config_.get().auto_config_
				, config_.get().group_
				, "mqtt"));

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

}
