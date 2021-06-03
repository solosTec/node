/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/cluster.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, std::string const& node_name
		, cyng::logger logger
		, toggle::server_vec_t&& cfg
		, bool client_login
		, std::chrono::seconds client_timeout
		, std::filesystem::path client_out)
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
		, store_()
		, db_(std::make_shared<db>(store_, logger_, tag_))
		, server_(ctl, logger, bus_, db_, client_timeout, client_out)
	{
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_name("connect", 0);
            sp->set_channel_name("listen", 1); //	wmbus_server
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    cluster::~cluster() {
#ifdef _DEBUG_BROKER_WMBUS
        std::cout << "cluster(~)" << std::endl;
#endif
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")");
        bus_.stop();
    }

    void cluster::connect() {
        //
        //	join cluster
        //
        bus_.start();
    }

    //
    //	bus interface
    //
    cyng::mesh *cluster::get_fabric() { return &fabric_; }
    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[cluster] join complete");

            //
            //	subscribe table "meterwMBus" and "meter"
            //
            auto slot = std::static_pointer_cast<cyng::slot_interface>(db_);
            db_->init(slot);
            // db_->loop([this](cyng::meta_store const& m) {
            //	bus_.req_subscribe(m.get_name());
            //	});
            bus_.req_subscribe("meter");

        } else {
            CYNG_LOG_ERROR(logger_, "[cluster] joining failed");
        }
    }

    void cluster::on_disconnect(std::string msg) {
        CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg);
        auto slot = std::static_pointer_cast<cyng::slot_interface>(db_);
        db_->disconnect(slot);
    }

    void
    cluster::db_res_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] insert: " << table_name << " - " << data);
        if (db_)
            db_->res_insert(table_name, key, data, gen, tag);
    }

    void cluster::db_res_trx(std::string table_name, bool trx) {

        CYNG_LOG_INFO(logger_, "[cluster] trx: " << table_name << (trx ? " start" : " commit"));
        if (!trx) {
            auto const next = db_->get_next_table(table_name);
            if (!next.empty()) {
                bus_.req_subscribe(next);
            }
        }
    }

    void
    cluster::db_res_update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] update: " << table_name << " - " << key);
        if (db_)
            db_->res_update(table_name, key, attr, gen, tag);
    }

    void cluster::db_res_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] remove: " << table_name << " - " << key);
        if (db_)
            db_->res_remove(table_name, key, tag);
    }

    void cluster::db_res_clear(std::string table_name, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] clear: " << table_name);
        if (db_)
            db_->res_clear(table_name, tag);
    }

} // namespace smf
