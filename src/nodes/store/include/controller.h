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

#include <cyng/db/session.h>

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
            std::chrono::seconds delay,
            std::set<std::string> const &,
            std::set<std::string> const &,
            std::set<std::string> const &,
            std::set<std::string> const &writer);

        void start_sml_db(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            cyng::db::session,
            cyng::param_map_t &&,
            std::string const &);

        void start_sml_xml(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string);
        void start_sml_json(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string);
        void start_sml_abl(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string,
            bool eol_dos);
        void start_sml_log(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string,
            bool);
        void start_sml_csv(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string,
            bool);
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

        void start_iec_db(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            cyng::db::session,
            cyng::param_map_t &&,
            std::string const &);
        void start_iec_json(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string);
        void start_iec_log(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string);
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
        void start_iec_csv(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &,
            std::string,
            std::string,
            std::string,
            bool);
        void start_dlms_influx(
            cyng::controller &,
            cyng::stash &channels,
            cyng::logger,
            std::string const &name,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db);

        void start_csv_reports(
            cyng::controller &ctl,
            cyng::stash &channels,
            cyng::logger logger,
            cyng::db::session,
            cyng::param_map_t reports);

        void start_lpex_reports(
            cyng::controller &ctl,
            cyng::stash &channels,
            cyng::logger logger,
            cyng::db::session,
            cyng::param_map_t reports,
            cyng::obis_path_t filter,
            bool print_version,
            bool debug_mode);

        void start_feed_reports(
            cyng::controller &ctl,
            cyng::stash &channels,
            cyng::logger logger,
            cyng::db::session,
            cyng::param_map_t reports,
            cyng::obis_path_t filter,
            bool print_version,
            bool debug_mode);

      private:
        /**
         * @param create if true tables will be created
         */
        std::map<std::string, cyng::db::session> init_storage(cyng::object const &, bool create);
        cyng::db::session init_storage(cyng::param_map_t const &);
        int create_influx_dbs(cyng::object const &, std::string const &cmd);
        void generate_csv_reports(cyng::object &&cfg);
        void generate_lpex_reports(cyng::object &&cfg);
        void generate_gap_reports(cyng::object &&cfg);
        void cleanup_archive(cyng::object &&cfg);
        void dump_readout(cyng::object &&cfg, std::chrono::hours);

        void start_cleanup_tasks(cyng::controller &ctl, cyng::logger, std::string, cyng::db::session, cyng::param_map_t &&);
        void start_gap_reports(cyng::controller &ctl, cyng::logger, std::string, cyng::db::session, cyng::param_map_t &&);
    };

    /**
     * Same function as in "report" app
     */
    cyng::prop_t create_report_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack);
    cyng::prop_t create_lpex_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack);
    cyng::prop_t create_cleanup_spec(cyng::obis profile, std::chrono::hours hours, bool enabled);
    cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled);

} // namespace smf

#endif
