/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_MAIN_CONTROLLER_H
#define SMF_MAIN_CONTROLLER_H

#include <smf/controller_base.h>

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
        cyng::param_t create_server_spec();
        cyng::param_t create_session_spec(std::filesystem::path const &tmp);
        cyng::param_t create_cluster_spec();

      private:
        cyng::channel_ptr cluster_;
    };
} // namespace smf

#endif
