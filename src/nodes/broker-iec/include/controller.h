/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_BROKER_IEC_CONTROLLER_H
#define SMF_BROKER_IEC_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>
#include <smf/ipt/config.h>

namespace smf {

    class controller : public config::controller_base {
      public:
        controller(config::startup const &);

      protected:
        cyng::vector_t create_default_config(
            std::chrono::system_clock::time_point &&now, std::filesystem::path &&tmp, std::filesystem::path &&cwd) override;

        virtual void
        run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) override;

        virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) override;

      private:
        void join_cluster(
            cyng::controller &ctl,
            cyng::stash &,
            cyng::logger logger,
            boost::uuids::uuid tag,
            std::string const &node_name,
            toggle::server_vec_t &&tgl,
            bool,
            std::string,
            std::filesystem::path);

        void join_network(
            cyng::controller &ctl,
            cyng::stash &,
            cyng::logger logger,
            boost::uuids::uuid tag,
            std::string const &node_name,
            ipt::toggle::server_vec_t &&tgl,
            ipt::push_channel &&);

      private:
        cyng::param_t create_client_spec();
        cyng::param_t create_cluster_spec();
        cyng::param_t create_ipt_spec(boost::uuids::uuid tag);
        cyng::param_t create_push_spec();
    };
} // namespace smf

#endif
