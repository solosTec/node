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

    class cluster : private bus_interface {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,
            // std::function<void(std::string, std::string)>, //	update client client
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
            std::filesystem::path out,
            std::size_t reconnect_timeout);

        void connect();

        void stop(cyng::eod);

      private:
        //
        //	bus interface
        //
        virtual cyng::mesh *get_fabric() override;
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
        void update_delay(std::uint32_t counter);
        void remove_iec_meter(cyng::key_t key);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        boost::uuids::uuid const tag_;
        cyng::logger logger_;
        std::filesystem::path const out_;
        std::size_t const reconnect_timeout_;

        cyng::mesh fabric_;
        bus bus_;
        cyng::store store_;
        std::shared_ptr<db> db_;
        std::chrono::seconds delay_;
        config::dependend_key dep_key_;
    };

    std::string make_task_name(std::string host, std::uint16_t port);

} // namespace smf

#endif
