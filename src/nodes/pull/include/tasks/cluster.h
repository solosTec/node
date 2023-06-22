/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_PULL_TASK_CLUSTER_H
#define SMF_PULL_TASK_CLUSTER_H

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/store/db.h>
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
            std::function<void(void)>, //
            // std::function<void(boost::asio::ip::tcp::endpoint)>,
            std::function<void(cyng::eod)> //
            >;

        class db_dispatcher : public cfg_db_interface {
          public:
            db_dispatcher(cluster *, cyng::channel_ptr);
            virtual void
            db_res_insert(std::string name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) override;

            virtual void db_res_trx(std::string name, bool start) override;
            virtual void
            db_res_update(std::string name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) override;
            virtual void db_res_remove(std::string name, cyng::key_t key, boost::uuids::uuid tag) override;
            virtual void db_res_clear(std::string name, boost::uuids::uuid tag) override;

            void subscribe();
            void open();

          private:
            cluster *cluster_;
            cyng::channel_ptr storage_db_;
        };
        friend db_dispatcher;

      public:
        cluster(
            cyng::channel_weak,
            cyng::controller &,
            boost::uuids::uuid tag,
            std::string const &node_name,
            cyng::logger,
            toggle::server_vec_t &&,
            cyng::param_map_t &&cfg_db);
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
        db_dispatcher db_dispatcher_;
    };

    cyng::channel_ptr start_data_store(cyng::controller &ctl, cyng::logger logger, bus &, cyng::param_map_t &&);

} // namespace smf

#endif
