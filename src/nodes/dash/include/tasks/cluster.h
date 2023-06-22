/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_TASK_CLUSTER_H
#define SMF_DASH_TASK_CLUSTER_H

#include <db.h>
#include <http_server.h>

#include <smf/cluster/bus.h>
#include <smf/http/auth.h>

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
            std::function<void(void)>,                           // open
            std::function<void(boost::asio::ip::tcp::endpoint)>, // listen
            std::function<void(cyng::eod)>                       // stop
            >;

      public:
        cluster(
            cyng::channel_weak,
            cyng::controller &,
            boost::uuids::uuid tag,
            std::string const &node_name,
            cyng::logger,
            toggle::server_vec_t &&,
            std::string const &document_root,
            std::uint64_t max_upload_size,
            std::string const &nickname,
            std::chrono::seconds timeout,
            std::string const &country_code,
            std::string const &lang_code,
            http_server::blocklist_type &&,
            std::map<std::string, std::string> &&,
            http::auth_dirs const &);
        ~cluster();

        void stop(cyng::eod);
        void connect();

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
        boost::uuids::uuid const tag_;
        cyng::logger logger_;
        cyng::mesh fabric_;
        bus bus_;
        cyng::store store_;
        db db_;
        http_server http_server_;
    };

} // namespace smf

#endif
