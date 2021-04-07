/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/cluster.h>
#include <notifier.h>

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
		, http_server::blocklist_type&& blocklist
		, std::map<std::string, std::string>&& redirects_intrinsic
		, http::auth_dirs const& auths)
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
			, bus_
			, logger
			, document_root
			, db_
			, std::move(blocklist)
			, std::move(redirects_intrinsic)
			, auths)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("connect", 0);
			sp->set_channel_name("listen", 1);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
		}

		auto slot = std::static_pointer_cast<cyng::slot_interface>(std::make_shared<notifier>(db_, http_server_, logger));
		db_.init(max_upload_size, nickname, timeout, slot);


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
			CYNG_LOG_INFO(logger_, "[cluster] join complete");

			//
			//	Subscribe tables
			//
			db_.loop_rel([&](db::rel const& r)->void {
				CYNG_LOG_INFO(logger_, "[cluster] subscribe table " << r.table_ << " (" << r.channel_ << ")");
				bus_.req_subscribe(r.table_);
			});

		}
		else {
			CYNG_LOG_ERROR(logger_, "[cluster] joining failed");
		}
	}

	void cluster::on_disconnect(std::string msg) {
		CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg);

		//
		//	clear cluster table
		//
		db_.cache_.clear("cluster", db_.cfg_.get_tag());
	}


	void cluster::db_res_insert(std::string table_name
		, cyng::key_t  key
		, cyng::data_t  data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		db_.res_insert(table_name
			, key
			, data
			, gen
			, tag);

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
	void cluster::db_res_update(std::string table_name
		, cyng::key_t key
		, cyng::attr_t attr
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		db_.res_update(table_name, key, attr, gen, tag);

	}

	void cluster::db_res_remove(std::string table_name
		, cyng::key_t key
		, boost::uuids::uuid tag) {

		db_.res_remove(table_name
			, key
			, tag);

	}

	void cluster::db_res_clear(std::string table_name
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] clear: "
			<< table_name);

		db_.res_clear(table_name
			, tag);

	}

}


