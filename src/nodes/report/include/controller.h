/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_REPORT_CONTROLLER_H
#define SMF_REPORT_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>

namespace smf {

    class controller : public config::controller_base {
      public:
        controller(config::startup const &);
        virtual bool run_options(boost::program_options::variables_map &) override;

      protected:
        cyng::vector_t create_default_config(
            std::chrono::system_clock::time_point &&now,
            std::filesystem::path &&tmp,
            std::filesystem::path &&cwd) override;
        virtual void
        run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) override;
        virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) override;

      private:
        void join_cluster(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            toggle::server_vec_t &&,
            cyng::param_map_t &&,
            cyng::param_map_t &&);

        /**
         * option --generate, -G
         */
        void generate_reports(cyng::object &&cfg);

      private:
        cyng::channel_ptr cluster_;
    };

    cyng::param_t create_cluster_spec();
    cyng::prop_t create_report_spec(cyng::obis, std::filesystem::path, bool enabled, std::chrono::hours);

} // namespace smf

#endif
