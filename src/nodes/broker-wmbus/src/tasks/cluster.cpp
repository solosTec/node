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
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
		}


	}

	cluster::~cluster()
	{
#ifdef _DEBUG_BROKER_WMBUS
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

	void cluster::on_disconnect(std::string msg) {
		CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg);
	}

	void cluster::db_res_insert(std::string table_name
		, cyng::key_t key
		, cyng::data_t data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] insert: "
			<< table_name
			<< " - "
			<< data);
	}

	void cluster::db_res_trx(std::string table_name
		, bool trx) {

		CYNG_LOG_TRACE(logger_, "[cluster] trx: "
			<< table_name
			<< (trx ? " start" : " commit"));
	}

	void cluster::db_res_update(std::string table_name
		, cyng::key_t key
		, cyng::attr_t attr
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] update: "
			<< table_name
			<< " - "
			<< key);

	}
	
	void cluster::db_res_remove(std::string table_name
		, cyng::key_t key
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] remove: "
			<< table_name
			<< " - "
			<< key);
	}

	void cluster::db_res_clear(std::string table_name
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] clear: "
			<< table_name);
	}

}


