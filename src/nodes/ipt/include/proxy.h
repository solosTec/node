/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_PROXY_H
#define SMF_IPT_PROXY_H

#include <smf/cluster/bus.h>
#include <smf/obis/tree.hpp>
#include <smf/sml/generator.h>
#include <smf/sml/unpack.h>

#include <cyng/log/logger.h>
#include <cyng/store/db.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>
#include <queue>

namespace smf {

    class ipt_session;

    /**
     * SML proxy
     */
    class proxy {

        enum class state { OFF, QRY_BASIC, QRY_METER, QRY_ACCESS, QRY_USER } state_;

      public:
        proxy(cyng::logger, ipt_session &, bus &cluster_bus, cyng::mac48 const &);
        ~proxy();

        bool is_on() const;
        void cfg_backup(std::string name, std::string pwd, cyng::buffer_t id);
        void read(cyng::buffer_t &&data);

      private:
        void get_proc_parameter_response(
            cyng::buffer_t const &,
            cyng::obis_path_t const &,
            cyng::obis,
            cyng::attr_t,
            cyng::tuple_t const &);
        void close_response(std::string trx, cyng::tuple_t const msg);
        void cfg_backup_meter();
        void cfg_backup_access();
        void cfg_backup_user();

        void complete();
        void update_connect_state(bool);

        std::string get_state() const;

        /**
         * send to configuration manager
         */
        void backup(boost::uuids::uuid tag, cyng::buffer_t gw, cyng::tuple_t cl);

      private:
        cyng::logger logger_;
        ipt_session &session_;
        bus &cluster_bus_;
        cyng::mac48 const client_id_;
        cyng::buffer_t id_; //  gateway id
        sml::unpack parser_;
        sml::request_generator req_gen_;
        std::queue<cyng::buffer_t> meters_;    //!< temporary data
        std::queue<cyng::obis_path_t> access_; //!< temporary data
        std::map<cyng::buffer_t, sml::tree> cfg_;
        cyng::store cache_;
        std::size_t throughput_;
    };

} // namespace smf

#endif
