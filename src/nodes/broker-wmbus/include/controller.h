/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_BROKER_WMBUS_CONTROLLER_H
#define SMF_BROKER_WMBUS_CONTROLLER_H

#include <smf/controller_base.h>
#include <smf/cluster/config.h>
#include <smf/ipt/config.h>

 namespace smf {

	class controller : public config::controller_base
	{
	public:
		controller(config::startup const&);

	protected:
		cyng::vector_t create_default_config(std::chrono::system_clock::time_point&& now
			, std::filesystem::path&& tmp
			, std::filesystem::path&& cwd) override;

		virtual void run(cyng::controller&
			, cyng::logger
			, cyng::object const& cfg
			, std::string const& node_name) override;

		virtual void shutdown(cyng::logger, cyng::registry&) override;

	private:
		void join_cluster(cyng::controller& ctl
			, cyng::logger logger
			, boost::uuids::uuid tag
			, std::string const& node_name
			, toggle::server_vec_t&& tgl
			, std::string const& address
			, std::uint16_t port
			, bool client_login
			, std::chrono::seconds client_timeout);

		void join_network(cyng::controller& ctl
			, cyng::logger logger
			, boost::uuids::uuid tag
			, std::string const& node_name
			, ipt::toggle::server_vec_t&& tgl
			, ipt::push_channel&&);

	private:
		cyng::param_t create_server_spec(std::filesystem::path const& cwd);
		cyng::param_t create_push_spec();
		cyng::param_t create_cluster_spec();
		cyng::param_t create_client_spec();
		cyng::param_t create_ipt_spec(boost::uuids::uuid tag);

	};
}

#endif