/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "join.h"
#include "../cli.h"
#include <smf/cluster/generator.h>

#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	join::join(cli* cp
		, cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, cluster_config_t const& cfg)
	: cli_(*cp)
		, bus_(bus_factory(mux, logger, tag, 0u))
		, logger_(logger)
		, config_(cfg)
	{
		cli_.vm_.register_function("join", 1, std::bind(&join::cmd, this, std::placeholders::_1));

		//
		//	implement request handler
		//
		bus_->vm_.register_function("bus.reconfigure", 1, std::bind(&join::reconfigure, this, std::placeholders::_1));

	}

	join::~join()
	{}

	void join::cmd(cyng::context& ctx)
	{


		auto const frame = ctx.get_frame();
		auto const cmd = cyng::value_cast<std::string>(frame.at(0), "list");
		if (boost::algorithm::equals(cmd, "help")) {

			cli_.out_
				<< "help\tprint this page" << std::endl
				<< "run\t\tjoin cluster" << std::endl
				<< "stop\t\tleave cluster" << std::endl
				<< "show\t\tshow  configuration" << std::endl
				;
		}
		else if (boost::algorithm::equals(cmd, "run")) {
			cli_.out_
				<< "run: "
				<< cmd
				<< std::endl;

			connect();

			//
			//	ToDo: start cluster task
			//

			//cyng::async::start_task_delayed<cluster>(mux
			//	, std::chrono::seconds(1)
			//	, logger
			//	, cluster_tag
			//	, load_cluster_cfg(cfg_cluster)
			//	, cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0")
			//	, cyng::value_cast<std::string>(dom.get("service"), "9000")
			//	, cyng::value_cast<int>(dom.get("timeout"), 12)
			//	, cyng::value_cast(dom.get("auto-answer"), true)
			//	, guard_time
			//	, blocklist);

		}
		else if (boost::algorithm::equals(cmd, "stop")) {
			cli_.out_
				<< "stop: "
				<< cmd
				<< std::endl;
		}
		else if (boost::algorithm::equals(cmd, "show")) {
			cli_.out_
				<< config_.get().account_
				<< ':'
				<< config_.get().pwd_
				<< '@'
				<< config_.get().host_
				<< ':'
				<< config_.get().service_
				<< std::endl;
		}
		else {
			cli_.out_
				<< "JOIN - unknown command: "
				<< cmd
				<< std::endl;
		}

	}

	void join::reconfigure(cyng::context& ctx)
	{
		reconfigure_impl();
	}


	void join::reconfigure_impl()
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
		//base_.suspend(config_.get().monitor_);

	}

	void join::connect()
	{

		BOOST_ASSERT_MSG(!bus_->vm_.is_halted(), "cluster bus is halted");
		bus_->vm_.async_run(bus_req_login(config_.get().host_
			, config_.get().service_
			, config_.get().account_
			, config_.get().pwd_
			, config_.get().auto_config_
			, config_.get().group_
			, "cli"));

		CYNG_LOG_INFO(logger_, "cluster login request is sent to "
			<< config_.get().host_
			<< ':'
			<< config_.get().service_);

	}

}