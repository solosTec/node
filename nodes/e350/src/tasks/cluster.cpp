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
	, master_(0)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running");

		//
		//	ToDo: implement request handler
		//

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
			, config_[master_].auto_config_
			, config_[master_].group_
			, "e350"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent");

	}

	void cluster::stop()
	{
        bus_->stop();
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

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
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

			bus_->vm_.async_run(bus_req_login(config_[master_].host_
				, config_[master_].service_
				, config_[master_].account_
				, config_[master_].pwd_
				, config_[master_].auto_config_
				, config_[master_].group_
				, "e350"));

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "cluster login failed - no other redundancy available");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

}
