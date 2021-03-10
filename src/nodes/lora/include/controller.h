/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>


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
		cyng::param_t create_server_spec(std::filesystem::path const& cwd);
		cyng::param_t create_cluster_spec();

	};
}