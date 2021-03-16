/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/cluster.h>
#include <tasks/storage_xml.h>
#include <tasks/storage_json.h>
#include <tasks/storage_db.h>

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
		, toggle::server_vec_t&& cfg
		, std::string storage_type
		, cyng::param_map_t&& cfg_db)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&cluster::stop, this, std::placeholders::_1)
		}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, fabric_(ctl)
		, bus_(ctl.get_ctx(), logger, std::move(cfg), node_name, tag, this)
		, store_()
		, storage_(start_data_store(ctl
			, logger
			, tag
			, store_
			, storage_type
			, std::move(cfg_db)))
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("connect", 0);

			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
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
		//	connect to database
		//
		storage_->dispatch("open", cyng::make_tuple());

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

			bus_.req_subscribe("TDevice");

		}

	}

	cyng::channel_ptr start_data_store(cyng::controller& ctl
		, cyng::logger logger
		, boost::uuids::uuid tag
		, cyng::store& cache
		, std::string const& storage_type
		, cyng::param_map_t&& cfg) {

		if (boost::algorithm::equals(storage_type, "DB")) {
			return ctl.create_named_channel_with_ref<storage_db>("storage", ctl, tag, cache, logger, std::move(cfg));
		}
		else if (boost::algorithm::equals(storage_type, "XML")) {
			CYNG_LOG_ERROR(logger, "XML data storage not available");
			return ctl.create_named_channel_with_ref<storage_xml>("storage", ctl, tag, cache, logger, std::move(cfg));
		}
		else if (boost::algorithm::equals(storage_type, "JSON")) {
			CYNG_LOG_ERROR(logger, "JSON data storage not available");
			return ctl.create_named_channel_with_ref<storage_json>("storage", ctl, tag, cache, logger, std::move(cfg));
		}

		CYNG_LOG_FATAL(logger, "no data storage configured");

		return cyng::channel_ptr();
	}



}


