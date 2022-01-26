/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_PROXY_H
#define SMF_IPT_PROXY_H

#include <smf/cluster/bus.h>
#include <smf/sml/generator.h>
#include <smf/sml/unpack.h>

#include <cyng/log/logger.h>
#include <cyng/store/db.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class ipt_session;

    /**
     * SML proxy
     */
    class proxy {

        enum class state { OFF, ON, ROOT_CFG, METER_CFG } state_;

      public:
        proxy(cyng::logger, ipt_session &, bus &cluster_bus, cyng::mac48 const &);
        ~proxy();

        bool is_on() const;
        void cfg_backup(std::string name, std::string pwd, cyng::buffer_t id);
        void read(cyng::buffer_t &&data);

      private:
        void get_proc_parameter_response(cyng::obis_path_t const &, cyng::obis, cyng::attr_t, cyng::tuple_t const &);
        void close_response(std::string trx, cyng::tuple_t const msg);
        void cfg_backup_meter();

      private:
        cyng::logger logger_;
        ipt_session &session_;
        bus &cluster_bus_;
        cyng::mac48 const client_id_;
        cyng::buffer_t id_; //  gateway id
        sml::unpack parser_;
        sml::request_generator req_gen_;
        std::set<cyng::buffer_t> meters_;
        cyng::store cache_;
    };

} // namespace smf

#endif
