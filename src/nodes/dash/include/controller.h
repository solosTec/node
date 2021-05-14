/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_DASH_CONTROLLER_H
#define SMF_DASH_CONTROLLER_H

#include <smf/cluster/config.h>
#include <smf/controller_base.h>
#include <smf/http/auth.h>

#include <vector>

#include <boost/asio/ip/address.hpp>

namespace smf {

    using blocklist_type = std::vector<boost::asio::ip::address>;

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
        cyng::param_t create_server_spec(std::filesystem::path const &root);
        cyng::param_t create_cluster_spec();
        cyng::param_t create_auth_spec();
        cyng::param_t create_block_list();
        cyng::param_t create_redirects();
        cyng::param_t create_intrinsic_redirects();

        void join_cluster(
            cyng::controller &,
            cyng::logger,
            boost::uuids::uuid,
            std::string const &node_name,
            toggle::server_vec_t &&,
            std::string const &address,
            std::uint16_t port,
            std::string const &document_root,
            std::uint64_t max_upload_size,
            std::string const &nickname,
            std::chrono::seconds timeout,
            std::string const &country_code,
            std::string const &lang_code,
            blocklist_type &&,
            std::map<std::string, std::string> &&,
            http::auth_dirs const &);

      private:
        cyng::channel_ptr cluster_;
    };

    blocklist_type convert_to_blocklist(std::vector<std::string> &&);

} // namespace smf

#endif
