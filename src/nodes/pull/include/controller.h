/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_PULL_CONTROLLER_H
#define SMF_PULL_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>

namespace smf {

    class controller : public config::controller_base {
      public:
        controller(config::startup const &);

        /**
         * Evaluate the transfer parameters
         */
        virtual bool run_options(boost::program_options::variables_map &) override;

        virtual void
        run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) override;

      protected:
        virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) override;

        cyng::vector_t create_default_config(
            std::chrono::system_clock::time_point &&now,
            std::filesystem::path &&tmp,
            std::filesystem::path &&cwd) override;

      private:
        cyng::param_t create_db_spec(std::filesystem::path cwd);
        cyng::param_t create_cluster_spec();

        void join_cluster(
            cyng::controller &,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            toggle::server_vec_t &&cfg_cluster,
            cyng::param_map_t &&cfg_db);

        void init_storage(cyng::object &&);

      private:
        /**
         * cluster bus
         */
        cyng::channel_ptr cluster_;
    };
} // namespace smf

#endif