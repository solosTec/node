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
		, toggle::server_vec_t&& cfg
		, std::string const& document_root
		, std::uint64_t max_upload_size
		, std::string const& nickname
		, std::chrono::seconds timeout
		, http_server::blocklist_type&& blocklist)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&http_server::listen, &http_server_, std::placeholders::_1),
		std::bind(&cluster::stop, this, std::placeholders::_1)
	}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, fabric_(ctl)
		, bus_(ctl.get_ctx(), logger, std::move(cfg), node_name, tag, this)
		, store_()
		, db_(store_, logger, tag)
		, http_server_(ctl.get_ctx()
			, tag
			, logger
			, document_root
			, db_
			, std::move(blocklist))
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("connect", 0);
			sp->set_channel_name("listen", 1);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
		}

		db_.init(max_upload_size, nickname, timeout);
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

			//
			//	Subscribe tables
			//
			bus_.req_subscribe("device");

		}
		else {
			CYNG_LOG_ERROR(logger_, "joining cluster failed");
		}
	}
	void cluster::db_res_subscribe(std::string table_name
		, cyng::key_t  key
		, cyng::data_t  data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		db_.db_res_subscribe(table_name, key, data, gen, tag);

	}
	void cluster::db_res_trx(std::string table_name
		, bool trx) {

		CYNG_LOG_INFO(logger_, "cluster trx: "
			<< table_name
			<< (trx ? " start" : " commit"));

		if (!trx) {
			//
			//	transfer complete
			//
			CYNG_LOG_INFO(logger_, "table size ["
				<< table_name
				<< "]: "
				<< store_.size(table_name));
		}
	}

}


