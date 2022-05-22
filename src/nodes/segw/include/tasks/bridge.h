/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_BRIDGE_H
#define SMF_SEGW_TASK_BRIDGE_H

#include <cfg.h>
#include <config/cfg_lmn.h>
#include <router.h>
#include <sml/server.h>
#include <storage.h>
#include <storage_functions.h>

#include <smf/ipt/config.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/store/db.h>
#include <cyng/task/stash.h>
#include <cyng/task/task_fwd.h>
#include <cyng/vm/mesh.h>

namespace smf {

    class cfg_listener;

    /**
     * manage segw lifetime
     */
    class bridge {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<std::function<void()>, std::function<void(cyng::eod)>>;

      public:
        bridge(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cyng::db::session, std::vector<cyng::meta_store> const &);

      private:
        void stop(cyng::eod);
        void start();

        // void init_data_cache();
        void load_config_data();

        void init_cache_persistence();
        void stop_cache_persistence();

        /**
         * Load tables with meter configuration into cache
         * and reset some initial values (like visiblity)
         */
        void load_meter();

        /**
         * Load specified table into cache
         */
        void load_table(cyng::meta_sql const &ms);

        void init_gpio();
        void stop_gpio();

        void init_virtual_meter();

        void init_sml_server();

        /**
         * NMS server is running as task
         */
        void init_nms_server();
        void stop_nms_server();
        bool test_nic(std::string const &nic);

        /**
         * start reading from serial ports
         */
        void init_lmn_ports();
        bool init_lmn_port(lmn_type);
        void stop_lmn_ports();
        void stop_lmn_port(lmn_type);

        void init_ipt_bus();
        void stop_ipt_bus();

        void init_broker_clients();
        void init_broker_clients(lmn_type);
        void stop_broker_clients();
        void stop_broker_clients(lmn_type);

        void init_filter();
        void init_filter(lmn_type);
        void stop_filter();
        void stop_filter(lmn_type);

        void init_redirectors();
        void init_redirector(lmn_type);
        void init_redirector_ipv6(cfg_listener const &cfg);
        void stop_redirectors();
        void stop_redirector(lmn_type);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::db::session db_;
        storage storage_;
        cyng::store cache_;
        cfg cfg_;
        cyng::mesh fabric_;
        router router_; //	SML router (incl. ip-t bus)
        sml::server sml_;
        cyng::stash stash_;
    };

    /**
     * Read table "TCfg" and transfer content to cache table "cfg".
     *
     * @return a tuple with the application tag and the server id.
     */
    std::tuple<boost::uuids::uuid, cyng::buffer_t> load_configuration(cyng::logger, cyng::db::session, cyng::store &);

    /**
     * Check if the specified index is a valid IPv6 NIC.
     */
    std::uint32_t validate_nic_index(std::uint32_t);

} // namespace smf

#endif
