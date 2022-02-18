/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_TASK_CLUSTER_H
#define SMF_DASH_TASK_CLUSTER_H

#include <db.h>

#include <smf/cluster/bus.h>
#include <smf/ipt/config.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/stash.h>
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

        using signatures_t = std::tuple<
            std::function<void(void)>, // connect
            std::function<void(cyng::eod)>>;

      public:
        cluster(
            cyng::channel_weak,
            cyng::controller &,
            boost::uuids::uuid tag,
            std::string const &node_name,
            cyng::logger,
            toggle::server_vec_t &&,
            bool login,
            std::size_t reconnect_timeout,
            ipt::toggle::server_vec_t &&,
            ipt::push_channel &&pcc);

        void connect();

        void stop(cyng::eod);

      private:
        //
        //	bus interface
        //
        virtual cyng::mesh *get_fabric() override;
        virtual cfg_interface *get_cfg_interface() override;
        virtual void on_login(bool) override;
        virtual void on_disconnect(std::string msg) override;
        virtual void
        db_res_insert(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) override;
        virtual void db_res_trx(std::string, bool) override;
        virtual void
        db_res_update(std::string, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) override;

        virtual void db_res_remove(std::string, cyng::key_t key, boost::uuids::uuid tag) override;

        virtual void db_res_clear(std::string, boost::uuids::uuid tag) override;

        /**
         * @return meter count
         */
        std::size_t check_gateway(cyng::record const &);
        void check_iec_meter(cyng::record const &);

        /**
         * The name of the meter is also the name of the task
         */
        std::size_t create_push_task(std::string const &, std::string const &, std::string const &, std::string const &client);
        void update_delay(std::uint32_t counter);
        void remove_iec_meter(cyng::key_t key);
        void remove_iec_gw(cyng::key_t key);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        boost::uuids::uuid const tag_;
        cyng::logger logger_;
        std::size_t const reconnect_timeout_;
        ipt::toggle::server_vec_t const cfg_ipt_;
        ipt::push_channel const pcc_;

        cyng::mesh fabric_;
        bus bus_;
        cyng::store store_;
        std::shared_ptr<db> db_;
        std::chrono::seconds delay_;
        config::dependend_key dep_key_;
        cyng::stash stash_;
    };

    std::string make_task_name(std::string host, std::uint16_t port);

    ipt::toggle::server_vec_t update_cfg(ipt::toggle::server_vec_t, std::string const &account, std::string const &pwd);

} // namespace smf

#endif
