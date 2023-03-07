/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */

#ifndef SMF_DNS_CONTROLLER_H
#define SMF_DNS_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>

namespace smf {

    /**
     * To change the used DNS server on windows use the following command:
     *
     * C:\WINDOWS\system32>netsh
     * netsh>interface ip set dns name="Ethernet 2" source="static" address="127.0.0.1"
     */
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
        cyng::param_t create_server_spec(std::filesystem::path const &cwd);
        cyng::param_t create_cluster_spec();

        void join_cluster(
            cyng::controller &,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            toggle::server_vec_t &&cfg,
            std::string &&address,
            std::uint16_t port);

      private:
        cyng::channel_ptr cluster_;
    };
} // namespace smf

#endif
