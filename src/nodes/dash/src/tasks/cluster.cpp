/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <notifier.h>
#include <tasks/cluster.h>

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
        std::string const &document_root,
        std::uint64_t max_upload_size,
        std::string const &nickname,
        std::chrono::seconds timeout,
        std::string const &country_code,
        std::string const &lang_code,
        http_server::blocklist_type &&blocklist,
        std::map<std::string, std::string> &&redirects_intrinsic,
        http::auth_dirs const &auths)
        : sigs_{
            std::bind(&cluster::connect, this), //  connect
            std::bind(&http_server::listen, &http_server_, std::placeholders::_1), //   listen (http_server)
            std::bind(&cluster::stop, this, std::placeholders::_1)  //  stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , tag_(tag)
        , logger_(logger)
        , fabric_(ctl)
        , bus_(ctl, logger, std::move(cfg), node_name, tag, CONFIG_MANAGER, this)
        , store_()
        , db_(store_, logger, tag)
        , http_server_(
              ctl.get_ctx(),
              bus_,
              logger,
              document_root,
              db_,
              std::move(blocklist),
              std::move(redirects_intrinsic),
              auths) {
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect", "listen"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }

        auto slot = std::static_pointer_cast<cyng::slot_interface>(std::make_shared<notifier>(db_, http_server_, logger));
        db_.init(max_upload_size, nickname, timeout, country_code, lang_code, slot);
    }

    cluster::~cluster() {
#ifdef _DEBUG_IPT
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
    cfg_db_interface *cluster::get_cfg_db_interface() { return &db_; };
    cfg_sink_interface *cluster::get_cfg_sink_interface() { return nullptr; }
    cfg_data_interface *cluster::get_cfg_data_interface() { return &http_server_; }

    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "[cluster] join complete");

            //
            //	Subscribe tables
            //
            db_.loop_rel([&](db::rel const &r) -> void {
                CYNG_LOG_INFO(logger_, "[cluster] subscribe table " << r.table_ << " (" << r.channel_ << ")");
                bus_.req_subscribe(r.table_);
            });

        } else {
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

} // namespace smf
