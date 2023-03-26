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
            cyng::param_map_t &&,
            cyng::param_map_t &&,
            cyng::param_map_t &&,
            cyng::param_map_t &&);

        /**
         * option --generate csv, -G
         */
        bool generate_csv_reports(cyng::object &&cfg);

        /**
         * option --generate lpex, -G
         */
        bool generate_lpex_reports(cyng::object &&cfg);

        /**
         * option --generate gap, -G
         */
        bool generate_gap_reports(cyng::object &&cfg);

        /**
         * option --generate feed, -G
         * Vorsch�be
         */
        bool generate_feed_reports(cyng::object &&cfg);

        void dump_readout(cyng::object &&cfg, std::chrono::hours backlog);

      private:
        cyng::channel_ptr cluster_;
    };

    cyng::param_t create_cluster_spec();

} // namespace smf

#endif
