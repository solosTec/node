/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_CONTROLLER_H
#define SMF_IPT_CONTROLLER_H

#include <smf/controller_base.h>
#include <smf/cluster/config.h>

 namespace smf {

	class controller : public config::controller_base
	{
	public:
		controller(config::startup const&);

	protected:
		cyng::vector_t create_default_config(std::chrono::system_clock::time_point&& now
			, std::filesystem::path&& tmp
			, std::filesystem::path&& cwd) override;
		void run(cyng::controller&
			, cyng::logger
			, cyng::object const& cfg
			, std::string const& node_name) override;

	private:
		cyng::param_t create_server_spec();
		cyng::param_t create_cluster_spec();

		void join_cluster(cyng::controller&
			, cyng::logger
			, boost::uuids::uuid
			, std::string const& node_name
			, toggle::server_vec_t&& cfg);
	};
}

#endif