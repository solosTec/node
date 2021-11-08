/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/cluster.h>
//#include <tasks/storage_xml.h>
//#include <tasks/storage_json.h>
//#include <tasks/storage_db.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        boost::uuids::uuid tag,
        std::string const &node_name,
        cyng::logger logger,
        toggle::server_vec_t &&cfg,
        std::string storage_type,
        cyng::param_map_t &&cfg_db)
        : sigs_{std::bind(&cluster::connect, this), std::bind(&cluster::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , tag_(tag)
        , logger_(logger)
        , fabric_(ctl)
        , bus_(ctl.get_ctx(), logger, std::move(cfg), node_name, tag, this)
        , store_() {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names({"connect"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    cluster::~cluster() {
#ifdef _DEBUG_REPORT
        std::cout << "cluster(~)" << std::endl;
#endif
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")");
        bus_.stop();
        // storage_->stop();
    }

    void cluster::connect() {
        //
        //	connect to database and
        //	load data into cache
        //
        // storage_->dispatch("open", cyng::make_tuple());

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
            CYNG_LOG_INFO(logger_, "cluster join complete");

            //
            //	load data from database and synchronize data with main node
            //
            // storage_->dispatch("load", cyng::make_tuple());
        } else {
            CYNG_LOG_ERROR(logger_, "joining cluster failed");
        }
    }
    void cluster::on_disconnect(std::string msg) { CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg); }

    void
    cluster::db_res_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());
        std::reverse(data.begin(), data.end());

        CYNG_LOG_TRACE(logger_, "[cluster] response insert/subscribe: " << table_name << " #" << gen << " <- " << data);

        store_.access(
            [&](cyng::table *tbl) {
                if (tbl->exist(key)) {

                    CYNG_LOG_INFO(logger_, "[cluster] remove duplicate: " << table_name << " <" << key << ">");
                    tbl->erase(key, tag);
                } else {

                    CYNG_LOG_INFO(logger_, "[cluster] make persistent: " << table_name << " " << key);

                    //
                    //	insert into database (persistence layer)
                    //
                    // storage_->dispatch("insert", cyng::make_tuple(table_name, key, data, gen, tag));
                }
            },
            cyng::access::write(table_name));
    }

    void cluster::db_res_trx(std::string table_name, bool trx) {

        CYNG_LOG_INFO(logger_, "[cluster] trx: " << table_name << (trx ? " start" : " commit"));

        if (!trx) {

            CYNG_LOG_INFO(logger_, "[cluster] upload: " << table_name << " #" << store_.size(table_name) << " records");

            upload(table_name);
        }
    }

    void
    cluster::db_res_update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(
            logger_,
            "[cluster] update " << table_name << ": " << key << " - " << attr.first //	zero based body index
                                << " => " << attr.second);

        // storage_->dispatch("update", cyng::make_tuple(table_name, key, attr, gen, tag));
    }

    void cluster::db_res_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] remove " << table_name << ": " << key);

        // storage_->dispatch("remove", cyng::make_tuple(table_name, key, tag));
    }

    void cluster::db_res_clear(std::string table_name, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] clear: " << table_name);

        // storage_->dispatch("clear", cyng::make_tuple(table_name, tag));
    }

    void cluster::upload(std::string const &table_name) {
        store_.access(
            [&](cyng::table *tbl) {
                tbl->loop([&](cyng::record &&rec, std::size_t idx) {
                    CYNG_LOG_TRACE(logger_, "[cluster] upload: " << table_name << " - " << rec.to_string());

                    bus_.req_db_insert(table_name, rec.key(), rec.data(), rec.get_generation());
                    return true;
                });

                //
                //	discard cache
                //
                CYNG_LOG_TRACE(logger_, "[cluster] discard: " << table_name);
                tbl->clear(tag_);
            },
            cyng::access::write(table_name));
    }

    // cyng::channel_ptr start_data_store(cyng::controller& ctl
    //	, cyng::logger logger
    //	, bus& cluster_bus
    //	, cyng::store& cache
    //	, std::string const& storage_type
    //	, cyng::param_map_t&& cfg) {

    //	if (boost::algorithm::equals(storage_type, "DB")) {
    //		return ctl.create_named_channel_with_ref<storage_db>("storage", ctl, cluster_bus, cache, logger, std::move(cfg));
    //	}
    //	else if (boost::algorithm::equals(storage_type, "XML")) {
    //		CYNG_LOG_ERROR(logger, "XML data storage not available");
    //		return ctl.create_named_channel_with_ref<storage_xml>("storage", ctl, cluster_bus, cache, logger, std::move(cfg));
    //	}
    //	else if (boost::algorithm::equals(storage_type, "JSON")) {
    //		CYNG_LOG_ERROR(logger, "JSON data storage not available");
    //		return ctl.create_named_channel_with_ref<storage_json>("storage", ctl, cluster_bus, cache, logger, std::move(cfg));
    //	}

    //	CYNG_LOG_FATAL(logger, "no data storage configured");

    //	return cyng::channel_ptr();
    //}

} // namespace smf
