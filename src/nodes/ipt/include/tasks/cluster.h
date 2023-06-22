/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_TASK_CLUSTER_H
#define SMF_IPT_TASK_CLUSTER_H

#include <smf/cluster/bus.h>
#include <smf/ipt/scramble_key.h>

#include <cyng/log/logger.h>
#include <cyng/net/server_proxy.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>
#include <cyng/vm/mesh.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class cluster : private bus_client_interface {
        template <typename T> friend class cyng::task;

        using signatures_t = std::
            tuple<std::function<void(void)>, std::function<void(boost::asio::ip::tcp::endpoint)>, std::function<void(cyng::eod)>>;

      public:
        cluster(
            cyng::channel_weak,
            cyng::controller &,
            boost::uuids::uuid tag,
            std::uint32_t query,
            std::string const &node_name,
            cyng::logger,
            toggle::server_vec_t &&,
            ipt::scramble_key const &sk,
            std::chrono::minutes watchdog,
            std::chrono::seconds timeout,
            cyng::mac48 client_id);
        ~cluster();

        void connect();

        void stop(cyng::eod);

      private:
        //
        //	bus interface
        //
        virtual cyng::mesh *get_fabric() override;
        virtual cfg_db_interface *get_cfg_db_interface() override;
        virtual cfg_sink_interface *get_cfg_sink_interface() override;
        virtual cfg_data_interface *get_cfg_data_interface() override;
        virtual void on_login(bool) override;
        virtual void on_disconnect(std::string msg) override;

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::mesh fabric_;

        /**
         * cluster bus
         */
        bus bus_;

        /**
         * IP-T server
         */
        cyng::net::server_proxy server_proxy_;
        std::uint64_t session_counter_;
    };

} // namespace smf

#endif
