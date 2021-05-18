/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_STORE_CONTROLLER_H
#define SMF_STORE_CONTROLLER_H

#include <smf/controller_base.h>
#include <smf/ipt/config.h>

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
        void join_network(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            std::string const &model,
            ipt::toggle::server_vec_t &&,
            std::vector<std::string> const &,
            std::vector<std::string> const &,
            std::vector<std::string> const &,
            std::vector<std::string> const &writer);

        void start_sml_db(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_sml_xml(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_sml_json(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_sml_abl(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_sml_log(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_sml_csv(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_sml_influx(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &name,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db);

        void start_iec_db(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_iec_log(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);
        void start_iec_influx(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &name,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db);
        void start_iec_csv(cyng::controller &, cyng::stash &channels, cyng::logger, cyng::param_map_t &&, std::string const &);

      private:
        void init_storage(cyng::object &&);
        int create_influx_dbs(cyng::object &&, std::string const &cmd);
    };
} // namespace smf

#endif
