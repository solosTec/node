/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/cluster.h>
#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& cfg)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&wmbus_server::listen, &server_, std::placeholders::_1),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, fabric_(ctl)
		, bus_(ctl.get_ctx(), logger, std::move(cfg), node_name, tag, this)
		, server_(ctl.get_ctx(), tag, logger)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("connect", 0);
			sp->set_channel_name("listen", 1);	//	wmbus_server
			CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");
		}


	}

	cluster::~cluster()
	{
#ifdef _DEBUG_IPT
		std::cout << "cluster(~)" << std::endl;
#endif
	}


	void cluster::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")");
		bus_.stop();
	}

	void cluster::connect()
	{
		//
		//	join cluster
		//
		bus_.start();

	}

	//
	//	bus interface
	//
	cyng::mesh* cluster::get_fabric() {
		return &fabric_;
	}
	void cluster::on_login(bool success) {
		if (success) {
			CYNG_LOG_INFO(logger_, "cluster join complete");

		}
		else {
			CYNG_LOG_ERROR(logger_, "joining cluster failed");
		}
	}
	void cluster::db_res_subscribe(std::string
		, cyng::key_t  key
		, cyng::data_t  data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {
	}

}


