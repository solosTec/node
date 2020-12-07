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

namespace node
{
	cluster::cluster(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cluster_config_t const& cfg_cluster
		, std::string const& address
		, std::string const& service
		, int timeout
		, bool auto_answer
		, std::chrono::milliseconds guard_time
		, std::set<boost::asio::ip::address> const& blocklist)
	: base_(*btp)
		, bus_(bus_factory(btp->mux_, logger, cluster_tag, btp->get_id()))
		, logger_(logger)
		, config_(cfg_cluster)
		, address_(address)
		, service_(service)
		, server_(btp->mux_, logger_, bus_, std::chrono::seconds(timeout), blocklist, auto_answer, guard_time)
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
		bus_->vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

	}

	cyng::continuation cluster::run()
	{	
		if (!bus_->is_online())
		{
			connect();
		}
		else
		{
			CYNG_LOG_DEBUG(logger_, "cluster bus is online");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void cluster::stop(bool shutdown)
	{
		//
		//	stop server
		//
		server_.close();

		//
		//	sign off from cluster
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
		//	start modem server
		//
		server_.run(address_, service_);

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation cluster::process()
	{
		//
		//	Connection to master lost
		//
		CYNG_LOG_WARNING(logger_, "lost connection to cluster");

		//
		//	stop server
		//
		server_.close();

		//
		//	switch to other configuration
		//
		reconfigure_impl();

		//
		//	continue task
		//
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
			, NODE::classes[NODE::class_e::_MODEM]));

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

}
