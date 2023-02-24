/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_IMEGA_CONTROLLER_H
#define SMF_IMEGA_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>
#include <smf/imega.h>

namespace smf {

    class controller : public config::controller_base {
      public:
        controller(config::startup const &);

      protected:
        cyng::vector_t create_default_config(
            std::chrono::system_clock::time_point &&now,
            std::filesystem::path &&tmp,
            std::filesystem::path &&cwd) override;
        virtual void
        run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) override;
        virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) override;

      private:
        cyng::param_t create_server_spec();
        cyng::param_t create_cluster_spec();

        void join_cluster(
            cyng::controller &,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            toggle::server_vec_t &&cfg,
            std::string const &address,
            std::uint16_t port,
            imega::policy,
            std::string const &pwd,
            std::chrono::seconds timeout,
            std::chrono::minutes watchdog);

      private:
        cyng::channel_ptr cluster_;
    };
} // namespace smf

#endif
