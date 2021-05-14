/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_SETUP_CONTROLLER_H
#define SMF_SETUP_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>

namespace smf {

    class controller : public config::controller_base {
      public:
        controller(config::startup const &);
        virtual bool run_options(boost::program_options::variables_map &) override;

      protected:
        cyng::vector_t create_default_config(
            std::chrono::system_clock::time_point &&now, std::filesystem::path &&tmp, std::filesystem::path &&cwd) override;
        virtual void
        run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) override;
        virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) override;

      private:
        cyng::param_t create_cluster_spec();

        void join_cluster(
            cyng::controller &,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            toggle::server_vec_t &&,
            std::string storage_type,
            cyng::param_map_t &&);

        void init_storage(cyng::object &&);
        void generate_access_rights(cyng::object &&cfg, std::string const &user);
        void generate_random_devs(cyng::object &&cfg, std::uint32_t count);

      private:
        cyng::channel_ptr cluster_;
    };
} // namespace smf

#endif
