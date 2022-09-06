/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */

#ifndef SMF_PULL_CONTROLLER_H
#define SMF_PULL_CONTROLLER_H

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
            std::set<std::string> const &targets);

      private:
        /**
         * @param create if true tables will be created
         */
        std::map<std::string, cyng::db::session> init_storage(cyng::object const &, bool create);
        cyng::db::session init_storage(cyng::param_map_t const &);
    };

} // namespace smf

#endif
